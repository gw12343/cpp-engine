//
// Created by gabe on 6/18/25.
//

#pragma once


#include <vector>
#include "glm/ext/matrix_clip_space.hpp"
#include "core/Window.h"
#include "Camera.h"
#include "rendering/Shader.h"
#include <vector>
#include <glm/glm.hpp>
#include "entt/entt.hpp"

namespace Engine {
	class ShadowMapRenderer {
	  public:
		void Initialize();
		void RenderShadowMaps();
		void UploadShadowMatrices(Engine::Shader& shader, glm::mat4& V, int textureSlot = 1);

	  private:

		std::vector<glm::vec4> getFrustumCornersWorldSpace(const glm::mat4& projview);
		std::vector<glm::vec4> getFrustumCornersWorldSpace(const glm::mat4& proj, const glm::mat4& view);
		glm::mat4              getLightSpaceMatrix(float nearPlane, float farPlane);
		std::vector<glm::mat4> getLightSpaceMatrices();


		static unsigned int lightFBO;
		static unsigned int matricesUBO;
		static unsigned int lightDepthMaps;

		Engine::Shader m_depthShader;
		Engine::Shader m_animationDepthShader;
	};

} // namespace Engine