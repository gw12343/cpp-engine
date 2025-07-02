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
		void Initalize();
		void RenderShadowMaps();
		void UploadShadowMatrices(Engine::Shader& shader, glm::mat4& V);

	  private:
		///////////////////////////////////////// SHADOW MAPPING CONSTANTS
		const static unsigned int depthMapResolution = 4096;
		const glm::vec3           lightDir           = glm::normalize(glm::vec3(2.0f, 20, 2.0f));
		const float               CAMERA_NEAR_PLANE  = 0.1f;
		const float               CAMERA_FAR_PLANE   = 1500.0f;
		std::vector<float>        shadowCascadeLevels{CAMERA_FAR_PLANE / 100.0f, CAMERA_FAR_PLANE / 50.0f, CAMERA_FAR_PLANE / 25.0f, CAMERA_FAR_PLANE / 10.0f, CAMERA_FAR_PLANE / 2.0f};


		std::vector<glm::vec4> getFrustumCornersWorldSpace(const glm::mat4& projview);
		std::vector<glm::vec4> getFrustumCornersWorldSpace(const glm::mat4& proj, const glm::mat4& view);
		glm::mat4              getLightSpaceMatrix(float nearPlane, float farPlane);
		std::vector<glm::mat4> getLightSpaceMatrices();


		static unsigned int lightFBO;
		static unsigned int matricesUBO;
		static unsigned int lightDepthMaps;

		Engine::Shader m_depthShader;
	};

} // namespace Engine