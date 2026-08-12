#pragma once

#include "rendering/Shader.h"

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <entt/entt.hpp>

typedef unsigned int GLuint;

namespace Engine {

	class FontAtlas;
	namespace Components {
		class Text3DComponent;
		class Transform;
	}

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

		// Editor mouse-picking: write EncodeEntityID colors into the picking FBO
		// for opaque letter coverage (same depth buffer as other pickables).
		void RenderMousePicking();

	  private:
		struct Vertex {
			glm::vec3 pos;
			glm::vec2 uv;
			glm::vec4 color;
		};

		void EnsureGpu();
		void Flush(FontAtlas* atlas, const std::vector<Vertex>& verts, float pxRange, float outlineWidth, const glm::vec3& outlineColor);

		// Append glyph quads for one Text3D into `out` (world-space positions).
		static void AppendTextGeometry(const Components::Text3DComponent& text,
		                               Components::Transform&             transform,
		                               FontAtlas*                         atlas,
		                               const glm::vec3&                   camRight,
		                               const glm::vec3&                   camUp,
		                               const glm::vec3&                   camPos,
		                               std::vector<Vertex>&               out);

		Shader m_shader;
		Shader m_pickingShader;
		GLuint m_vao   = 0;
		GLuint m_vbo   = 0;
		bool   m_ready = false;
	};

} // namespace Engine
