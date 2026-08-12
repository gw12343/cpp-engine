#include "Text3DRenderer.h"
#include "FontAtlas.h"

#include "components/impl/Text3DComponent.h"
#include "components/impl/TransformComponent.h"
#include "components/impl/EntityMetadataComponent.h"
#include "core/EngineData.h"
#include "Camera.h"

#include <glm/gtc/matrix_transform.hpp>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace Engine {

	namespace {
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
			return c;
		}

		// Split on '\n', keep empty lines.
		std::vector<std::string> SplitLines(const std::string& text)
		{
			std::vector<std::string> lines;
			size_t                   start = 0;
			for (size_t i = 0; i <= text.size(); ++i) {
				if (i == text.size() || text[i] == '\n') {
					lines.emplace_back(text.substr(start, i - start));
					start = i + 1;
				}
			}
			if (lines.empty()) lines.emplace_back("");
			return lines;
		}
	} // namespace

	Text3DRenderer::~Text3DRenderer()
	{
		Shutdown();
	}

	void Text3DRenderer::Initialize()
	{
		ReloadShaders();
		EnsureGpu();
		m_ready = true;
	}

	void Text3DRenderer::Shutdown()
	{
		if (m_vao != 0 && glfwGetCurrentContext() != nullptr) {
			glDeleteVertexArrays(1, &m_vao);
			glDeleteBuffers(1, &m_vbo);
		}
		m_vao = 0;
		m_vbo = 0;
		m_shader.Destroy();
		FontAtlasCache::Instance().Clear();
		m_ready = false;
	}

	void Text3DRenderer::ReloadShaders()
	{
		if (!m_shader.LoadFromFiles("resources/shaders/text/text3d_vert.glsl",
		                            "resources/shaders/text/text3d_frag.glsl",
		                            std::nullopt)) {
			GetDefaultLogger()->error("Text3DRenderer: failed to load shaders");
		}
	}

	void Text3DRenderer::EnsureGpu()
	{
		if (m_vao != 0) return;

		glGenVertexArrays(1, &m_vao);
		glGenBuffers(1, &m_vbo);
		glBindVertexArray(m_vao);
		glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * 1024, nullptr, GL_DYNAMIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	void Text3DRenderer::Flush(FontAtlas* atlas, const std::vector<Vertex>& verts,
	                           float pxRange, float outlineWidth, const glm::vec3& outlineColor)
	{
		if (!atlas || !atlas->IsValid() || verts.empty()) return;

		EnsureGpu();

		glBindVertexArray(m_vao);
		glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
		glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(verts.size() * sizeof(Vertex)), verts.data(), GL_DYNAMIC_DRAW);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, atlas->GetTextureID());

		m_shader.SetInt("uAtlas", 0);
		m_shader.SetFloat("uPxRange", pxRange);
		m_shader.SetFloat("uOutlineWidth", outlineWidth);
		m_shader.SetVec3("uOutlineColor", outlineColor);

		glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(verts.size()));

		glBindVertexArray(0);
	}

	void Text3DRenderer::Render()
	{
		if (!m_ready) return;
		if (m_shader.GetProgramID() == 0) return;

		auto& registry = GetCurrentSceneRegistry();
		auto  view     = registry.view<Components::EntityMetadata, Components::Transform, Components::Text3DComponent>();
		if (view.begin() == view.end()) return;

		const glm::mat4 viewMat = GetCamera().GetViewMatrix();
		const glm::mat4 projMat = GetCamera().GetProjectionMatrix();
		const glm::mat4 invView = glm::inverse(viewMat);
		const glm::vec3 camRight = glm::normalize(glm::vec3(invView[0]));
		const glm::vec3 camUp    = glm::normalize(glm::vec3(invView[1]));
		const glm::vec3 camPos   = glm::vec3(invView[3]);

		// Transparent pass: depth test on, no depth write, alpha blend.
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);
		glDisable(GL_CULL_FACE);

		m_shader.Bind();
		glm::mat4 vp = projMat * viewMat;
		m_shader.SetMat4("uViewProj", &vp);

		// Batch by (font path, outline) within a single pass — flush when atlas/outline changes.
		FontAtlas*          currentAtlas = nullptr;
		float               currentOutline = -1.f;
		glm::vec3           currentOutlineCol{0.f};
		std::vector<Vertex> batch;
		batch.reserve(512);

		auto flushIfNeeded = [&](FontAtlas* atlas, float outlineW, const glm::vec3& outlineCol) {
			const bool change = atlas != currentAtlas || outlineW != currentOutline || outlineCol != currentOutlineCol;
			if (change && currentAtlas && !batch.empty()) {
				// Match FreeType bsdf spread used when baking the atlas.
				const float pxRange = 8.0f;
				Flush(currentAtlas, batch, pxRange, currentOutline, currentOutlineCol);
				batch.clear();
			}
			currentAtlas      = atlas;
			currentOutline    = outlineW;
			currentOutlineCol = outlineCol;
		};

		for (auto entity : view) {
			const auto& meta = view.get<Components::EntityMetadata>(entity);
			if (!meta.active) continue;

			auto& text = view.get<Components::Text3DComponent>(entity);
			if (text.text.empty()) continue;

			FontAtlas* atlas = FontAtlasCache::Instance().GetOrLoad(text.fontPath, text.atlasPixelHeight);
			if (!atlas) continue;

			flushIfNeeded(atlas, text.outlineWidth, text.outlineColor);

			auto&             transform = view.get<Components::Transform>(entity);
			const glm::vec3   origin    = transform.GetWorldPosition();
			const float       worldSize = std::max(0.001f, text.size);
			const float       scale     = worldSize / atlas->GetPixelSize(); // world units per atlas pixel

			// Basis: billboard faces camera, otherwise use entity orientation.
			glm::vec3 right, up;
			if (text.billboard) {
				right = camRight;
				up    = camUp;
				// Optional: keep upright relative to world up for museum labels.
				if (text.billboardYLock) {
					up    = glm::vec3(0.f, 1.f, 0.f);
					right = glm::normalize(glm::cross(up, camPos - origin));
					if (glm::length(right) < 1e-4f) right = camRight;
					up = glm::normalize(glm::cross(right, glm::normalize(camPos - origin)));
				}
			} else {
				const glm::quat rot = transform.GetWorldRotation();
				right               = rot * glm::vec3(1.f, 0.f, 0.f);
				up                  = rot * glm::vec3(0.f, 1.f, 0.f);
			}

			const auto  lines = SplitLines(text.text);
			const float lineH = atlas->GetLineHeight() * scale;
			// Vertical start: first baseline so block is roughly centered on entity for multi-line.
			const float totalH = lineH * static_cast<float>(lines.size());
			float       penY   = totalH * 0.5f - atlas->GetAscent() * scale;

			for (const auto& line : lines) {
				const float lineW = atlas->MeasureWidth(line) * scale;
				float       penX  = 0.f;
				if (text.alignment == 1) penX = -lineW * 0.5f;      // center
				else if (text.alignment == 2) penX = -lineW;        // right
				// alignment 0 = left, penX = 0

				size_t i = 0;
				while (i < line.size()) {
					const uint32_t cp = DecodeUtf8(line, i);
					const Glyph*   g  = atlas->GetGlyph(cp);
					if (!g) continue;

					if (g->width > 0.f && g->height > 0.f) {
						const float x0 = penX + g->bearingX * scale;
						const float y0 = penY + (g->bearingY - g->height) * scale; // bottom of glyph
						const float x1 = x0 + g->width * scale;
						const float y1 = y0 + g->height * scale; // top of glyph

						const glm::vec3 p00 = origin + right * x0 + up * y0;
						const glm::vec3 p10 = origin + right * x1 + up * y0;
						const glm::vec3 p11 = origin + right * x1 + up * y1;
						const glm::vec3 p01 = origin + right * x0 + up * y1;

						const glm::vec4 col = text.color;

						// Two triangles: 00-10-11, 00-11-01
						batch.push_back({p00, {g->u0, g->v1}, col});
						batch.push_back({p10, {g->u1, g->v1}, col});
						batch.push_back({p11, {g->u1, g->v0}, col});

						batch.push_back({p00, {g->u0, g->v1}, col});
						batch.push_back({p11, {g->u1, g->v0}, col});
						batch.push_back({p01, {g->u0, g->v0}, col});
					}

					penX += g->advance * scale + text.letterSpacing;
				}

				penY -= lineH;
			}
		}

		if (currentAtlas && !batch.empty()) {
			// FreeType bsdf spread is 8 atlas px by default — used as AA floor scale.
			Flush(currentAtlas, batch, 8.0f, currentOutline, currentOutlineCol);
		}

		glDepthMask(GL_TRUE);
		glDisable(GL_BLEND);
		glEnable(GL_CULL_FACE);
	}

} // namespace Engine
