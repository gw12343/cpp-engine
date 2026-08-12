#include "FontAtlas.h"

#include "core/EngineData.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_IMAGE_H
#include FT_MODULE_H

#include <algorithm>
#include <cstring>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace Engine {

	namespace {
		// Decode one UTF-8 codepoint; advances i past the consumed bytes.
		uint32_t DecodeUtf8(const std::string& s, size_t& i)
		{
			if (i >= s.size()) return 0;
			const unsigned char c = static_cast<unsigned char>(s[i]);
			if (c < 0x80) {
				++i;
				return c;
			}
			if ((c & 0xE0) == 0xC0 && i + 1 < s.size()) {
				uint32_t cp = (c & 0x1F) << 6;
				cp |= (static_cast<unsigned char>(s[i + 1]) & 0x3F);
				i += 2;
				return cp;
			}
			if ((c & 0xF0) == 0xE0 && i + 2 < s.size()) {
				uint32_t cp = (c & 0x0F) << 12;
				cp |= (static_cast<unsigned char>(s[i + 1]) & 0x3F) << 6;
				cp |= (static_cast<unsigned char>(s[i + 2]) & 0x3F);
				i += 3;
				return cp;
			}
			if ((c & 0xF8) == 0xF0 && i + 3 < s.size()) {
				uint32_t cp = (c & 0x07) << 18;
				cp |= (static_cast<unsigned char>(s[i + 1]) & 0x3F) << 12;
				cp |= (static_cast<unsigned char>(s[i + 2]) & 0x3F) << 6;
				cp |= (static_cast<unsigned char>(s[i + 3]) & 0x3F);
				i += 4;
				return cp;
			}
			++i;
			return c; // invalid sequence: skip one byte
		}
	} // namespace

	FontAtlas::~FontAtlas()
	{
		Destroy();
	}

	FontAtlas::FontAtlas(FontAtlas&& other) noexcept
	    : m_path(std::move(other.m_path))
	    , m_glyphs(std::move(other.m_glyphs))
	    , m_textureID(other.m_textureID)
	    , m_atlasWidth(other.m_atlasWidth)
	    , m_atlasHeight(other.m_atlasHeight)
	    , m_pixelHeight(other.m_pixelHeight)
	    , m_lineHeight(other.m_lineHeight)
	    , m_ascent(other.m_ascent)
	{
		other.m_textureID = 0;
	}

	FontAtlas& FontAtlas::operator=(FontAtlas&& other) noexcept
	{
		if (this != &other) {
			Destroy();
			m_path        = std::move(other.m_path);
			m_glyphs      = std::move(other.m_glyphs);
			m_textureID   = other.m_textureID;
			m_atlasWidth  = other.m_atlasWidth;
			m_atlasHeight = other.m_atlasHeight;
			m_pixelHeight = other.m_pixelHeight;
			m_lineHeight  = other.m_lineHeight;
			m_ascent      = other.m_ascent;
			other.m_textureID = 0;
		}
		return *this;
	}

	void FontAtlas::Destroy()
	{
		if (m_textureID != 0 && glfwGetCurrentContext() != nullptr) {
			glDeleteTextures(1, &m_textureID);
		}
		m_textureID = 0;
		m_glyphs.clear();
	}

	bool FontAtlas::Load(const std::string& fontPath, int pixelHeight)
	{
		Destroy();
		m_path        = fontPath;
		m_pixelHeight = std::max(8, pixelHeight);

		FT_Library library = nullptr;
		if (FT_Init_FreeType(&library) != 0) {
			GetDefaultLogger()->error("FontAtlas: FT_Init_FreeType failed for '{}'", fontPath);
			return false;
		}

		FT_Face face = nullptr;
		if (FT_New_Face(library, fontPath.c_str(), 0, &face) != 0) {
			GetDefaultLogger()->error("FontAtlas: failed to load font '{}'", fontPath);
			FT_Done_FreeType(library);
			return false;
		}

		if (FT_Set_Pixel_Sizes(face, 0, static_cast<FT_UInt>(m_pixelHeight)) != 0) {
			GetDefaultLogger()->error("FontAtlas: FT_Set_Pixel_Sizes failed for '{}'", fontPath);
			FT_Done_Face(face);
			FT_Done_FreeType(library);
			return false;
		}

		// FreeType outline-SDF (module "sdf") is sensitive to sharp joins and small
		// features — that shows up as dark cracks inside connected strokes.
		// Prefer the more stable bitmap→SDF path ("bsdf"): render AA first, then SDF.
		// Spread is the max distance encoded into 0..255 (padding must cover this).
		const FT_Int sdfSpread = 8;
		FT_Property_Set(library, "sdf", "spread", &sdfSpread);
		FT_Property_Set(library, "bsdf", "spread", &sdfSpread);

		m_ascent     = static_cast<float>(face->size->metrics.ascender) / 64.f;
		m_lineHeight = static_cast<float>(face->size->metrics.height) / 64.f;

		// Characters to bake: printable ASCII + common Latin-1 supplements used in labels.
		std::vector<uint32_t> codepoints;
		codepoints.reserve(200);
		for (uint32_t c = 32; c < 127; ++c) codepoints.push_back(c);
		// A few extras (space already included).
		const uint32_t extras[] = {0x00A0, 0x00B0, 0x2013, 0x2014, 0x2018, 0x2019, 0x201C, 0x201D, 0x2026};
		for (uint32_t c : extras) codepoints.push_back(c);

		struct RasterGlyph {
			uint32_t              codepoint = 0;
			int                   width = 0, height = 0;
			int                   bearingX = 0, bearingY = 0;
			float                 advance = 0.f;
			std::vector<uint8_t>  pixels;
		};

		std::vector<RasterGlyph> rasters;
		rasters.reserve(codepoints.size());

		// Extra empty border so linear filtering / SDF falloff never samples a neighbour.
		const int padding = sdfSpread + 2;

		for (uint32_t cp : codepoints) {
			// Load outline; rasterize greyscale first so FT_RENDER_MODE_SDF uses bsdf.
			if (FT_Load_Char(face, cp, FT_LOAD_DEFAULT) != 0) {
				continue;
			}

			FT_Error renderErr = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
			if (renderErr == 0) {
				// Re-render the existing bitmap into an SDF (bsdf module).
				renderErr = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_SDF);
			}
			if (renderErr != 0) {
				// Last resort: outline SDF or skip.
				if (FT_Load_Char(face, cp, FT_LOAD_DEFAULT) != 0) continue;
				renderErr = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_SDF);
			}
			if (renderErr != 0) {
				continue;
			}

			const FT_Bitmap& bmp = face->glyph->bitmap;

			RasterGlyph rg;
			rg.codepoint = cp;
			rg.width     = static_cast<int>(bmp.width);
			rg.height    = static_cast<int>(bmp.rows);
			rg.bearingX  = face->glyph->bitmap_left;
			rg.bearingY  = face->glyph->bitmap_top;
			rg.advance   = static_cast<float>(face->glyph->advance.x) / 64.f;

			if (rg.width > 0 && rg.height > 0 && bmp.buffer) {
				rg.pixels.resize(static_cast<size_t>(rg.width * rg.height));
				// FreeType rows may have pitch != width (and pitch may be negative).
				for (int y = 0; y < rg.height; ++y) {
					const uint8_t* src = bmp.buffer + y * bmp.pitch;
					uint8_t*       dst = rg.pixels.data() + y * rg.width;
					std::memcpy(dst, src, static_cast<size_t>(rg.width));
				}
			}

			rasters.push_back(std::move(rg));
		}

		// Simple row packing into a square-ish atlas.
		int atlasW = 512;
		int atlasH = 512;
		// Estimate required area and grow if needed.
		{
			int totalArea = 0;
			for (const auto& r : rasters) {
				totalArea += (r.width + padding * 2) * (r.height + padding * 2);
			}
			while (atlasW * atlasH < totalArea * 2 && atlasW < 4096) {
				if (atlasW <= atlasH) atlasW *= 2;
				else atlasH *= 2;
			}
		}

		// Pack with growing height if rows overflow.
		std::vector<uint8_t> atlas(static_cast<size_t>(atlasW * atlasH), 0);
		int                  penX = padding;
		int                  penY = padding;
		int                  rowH = 0;

		auto ensureSpace = [&](int w, int h) {
			if (penX + w + padding > atlasW) {
				penX = padding;
				penY += rowH + padding;
				rowH = 0;
			}
			if (penY + h + padding > atlasH) {
				// Grow atlas height.
				int newH = atlasH;
				while (penY + h + padding > newH && newH < 8192) newH *= 2;
				if (newH != atlasH) {
					std::vector<uint8_t> grown(static_cast<size_t>(atlasW * newH), 0);
					for (int y = 0; y < atlasH; ++y) {
						std::memcpy(grown.data() + y * atlasW, atlas.data() + y * atlasW, static_cast<size_t>(atlasW));
					}
					atlas   = std::move(grown);
					atlasH  = newH;
				}
			}
		};

		for (auto& r : rasters) {
			const int gw = r.width;
			const int gh = r.height;
			ensureSpace(gw, gh);

			// Blit glyph into atlas.
			for (int y = 0; y < gh; ++y) {
				for (int x = 0; x < gw; ++x) {
					atlas[static_cast<size_t>((penY + y) * atlasW + (penX + x))] =
					    r.pixels[static_cast<size_t>(y * gw + x)];
				}
			}

			Glyph g;
			g.advance  = r.advance;
			g.bearingX = static_cast<float>(r.bearingX);
			g.bearingY = static_cast<float>(r.bearingY);
			g.width    = static_cast<float>(gw);
			g.height   = static_cast<float>(gh);
			g.u0       = static_cast<float>(penX) / static_cast<float>(atlasW);
			g.v0       = static_cast<float>(penY) / static_cast<float>(atlasH);
			g.u1       = static_cast<float>(penX + gw) / static_cast<float>(atlasW);
			g.v1       = static_cast<float>(penY + gh) / static_cast<float>(atlasH);
			m_glyphs[r.codepoint] = g;

			penX += gw + padding;
			rowH = std::max(rowH, gh);
		}

		// Always provide a space glyph even if face lacked it.
		if (m_glyphs.find(32) == m_glyphs.end()) {
			Glyph space;
			space.advance = static_cast<float>(m_pixelHeight) * 0.3f;
			m_glyphs[32]  = space;
		}

		m_atlasWidth  = atlasW;
		m_atlasHeight = atlasH;

		glGenTextures(1, &m_textureID);
		glBindTexture(GL_TEXTURE_2D, m_textureID);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, atlasW, atlasH, 0, GL_RED, GL_UNSIGNED_BYTE, atlas.data());
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		// Sample .r correctly when texture is GL_RED
		GLint swizzleMask[] = {GL_RED, GL_RED, GL_RED, GL_RED};
		glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
		glBindTexture(GL_TEXTURE_2D, 0);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

		FT_Done_Face(face);
		FT_Done_FreeType(library);

		GetDefaultLogger()->info("FontAtlas: loaded '{}' ({}x{}, {} glyphs, px={})",
		                         fontPath, atlasW, atlasH, m_glyphs.size(), m_pixelHeight);
		return true;
	}

	const Glyph* FontAtlas::GetGlyph(uint32_t codepoint) const
	{
		auto it = m_glyphs.find(codepoint);
		if (it != m_glyphs.end()) return &it->second;
		// Fallback to '?' then space.
		it = m_glyphs.find('?');
		if (it != m_glyphs.end()) return &it->second;
		it = m_glyphs.find(' ');
		if (it != m_glyphs.end()) return &it->second;
		return nullptr;
	}

	float FontAtlas::MeasureWidth(const std::string& text) const
	{
		float  w = 0.f;
		size_t i = 0;
		while (i < text.size()) {
			const uint32_t cp = DecodeUtf8(text, i);
			if (cp == '\n') continue;
			const Glyph* g = GetGlyph(cp);
			if (g) w += g->advance;
		}
		return w;
	}

	// ---------------------------------------------------------------------------

	FontAtlasCache& FontAtlasCache::Instance()
	{
		static FontAtlasCache cache;
		return cache;
	}

	FontAtlas* FontAtlasCache::GetOrLoad(const std::string& fontPath, int pixelHeight)
	{
		// Key includes pixel height so different sizes get separate atlases.
		const std::string key = fontPath + "#" + std::to_string(pixelHeight);
		auto              it  = m_atlases.find(key);
		if (it != m_atlases.end()) {
			return it->second.IsValid() ? &it->second : nullptr;
		}

		FontAtlas atlas;
		if (!atlas.Load(fontPath, pixelHeight)) {
			m_atlases.emplace(key, FontAtlas{}); // cache failure as empty
			return nullptr;
		}
		auto [ins, _] = m_atlases.emplace(key, std::move(atlas));
		return &ins->second;
	}

	void FontAtlasCache::Clear()
	{
		m_atlases.clear();
	}

} // namespace Engine
