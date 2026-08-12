#pragma once

#include "rendering/Shader.h"

#include <string>
#include <vector>
#include <glm/glm.hpp>

typedef unsigned int GLuint;

namespace Engine {

	class FontAtlas;

	// Batched transparent SDF text drawn after deferred lighting into GAME_OUT.
	class Text3DRenderer {
	  public:
		Text3DRenderer() = default;
		~Text3DRenderer();

		void Initialize();
		void Shutdown();
		void ReloadShaders();

		// Draw all Text3D components in the current scene.
		void Render();

	  private:
		struct Vertex {
			glm::vec3 pos;
			glm::vec2 uv;
			glm::vec4 color;
		};

		void EnsureGpu();
		void Flush(FontAtlas* atlas, const std::vector<Vertex>& verts, float pxRange, float outlineWidth, const glm::vec3& outlineColor);

		Shader m_shader;
		GLuint m_vao = 0;
		GLuint m_vbo = 0;
		bool   m_ready = false;
	};

} // namespace Engine
