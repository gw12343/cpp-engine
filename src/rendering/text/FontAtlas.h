#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>

typedef unsigned int GLuint;

namespace Engine {

	struct Glyph {
		float advance  = 0.f; // horizontal advance in atlas pixels
		float bearingX = 0.f;
		float bearingY = 0.f; // top of glyph relative to baseline
		float width    = 0.f;
		float height   = 0.f;
		float u0 = 0.f, v0 = 0.f, u1 = 0.f, v1 = 0.f;
	};

	// FreeType-backed greyscale / SDF font atlas for world-space 3D text.
	class FontAtlas {
	  public:
		FontAtlas() = default;
		~FontAtlas();

		FontAtlas(const FontAtlas&)            = delete;
		FontAtlas& operator=(const FontAtlas&) = delete;
		FontAtlas(FontAtlas&& other) noexcept;
		FontAtlas& operator=(FontAtlas&& other) noexcept;

		// pixelHeight is the FreeType face size used when baking the atlas.
		bool Load(const std::string& fontPath, int pixelHeight = 64);

		void Destroy();

		[[nodiscard]] const Glyph* GetGlyph(uint32_t codepoint) const;
		[[nodiscard]] float        MeasureWidth(const std::string& text) const; // atlas pixels
		[[nodiscard]] float        GetLineHeight() const { return m_lineHeight; }
		[[nodiscard]] float        GetAscent() const { return m_ascent; }
		[[nodiscard]] float        GetPixelSize() const { return static_cast<float>(m_pixelHeight); }
		[[nodiscard]] GLuint       GetTextureID() const { return m_textureID; }
		[[nodiscard]] int          GetAtlasWidth() const { return m_atlasWidth; }
		[[nodiscard]] int          GetAtlasHeight() const { return m_atlasHeight; }
		[[nodiscard]] bool         IsValid() const { return m_textureID != 0; }
		[[nodiscard]] const std::string& GetPath() const { return m_path; }

	  private:
		std::string                          m_path;
		std::unordered_map<uint32_t, Glyph>  m_glyphs;
		GLuint                               m_textureID  = 0;
		int                                  m_atlasWidth = 0;
		int                                  m_atlasHeight = 0;
		int                                  m_pixelHeight = 64;
		float                                m_lineHeight  = 0.f;
		float                                m_ascent      = 0.f;
	};

	// Cached atlases shared across Text3D draws.
	class FontAtlasCache {
	  public:
		static FontAtlasCache& Instance();

		// Returns a valid atlas or nullptr on failure.
		FontAtlas* GetOrLoad(const std::string& fontPath, int pixelHeight = 64);
		void       Clear();

	  private:
		FontAtlasCache() = default;
		std::unordered_map<std::string, FontAtlas> m_atlases;
	};

} // namespace Engine
