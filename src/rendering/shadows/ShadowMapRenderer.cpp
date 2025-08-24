//
// Created by gabe on 6/18/25.
//

#include "ShadowMapRenderer.h"
#include "glm/ext/matrix_transform.hpp"
#include "spdlog/spdlog.h"
#include "utils/Utils.h"
#include "glm/gtc/type_ptr.hpp"
#include "physics/PhysicsManager.h"
#include "core/EngineData.h"
#include "components/impl/EntityMetadataComponent.h"
#include "components/impl/ModelRendererComponent.h"
#include "components/impl/ShadowCasterComponent.h"
#include <glad/glad.h>

namespace Engine {

	unsigned int ShadowMapRenderer::lightFBO;
	unsigned int ShadowMapRenderer::matricesUBO;
	unsigned int ShadowMapRenderer::lightDepthMaps;


	// https://learnopengl.com/Guest-Articles/2021/CSM
	std::vector<glm::vec4> ShadowMapRenderer::getFrustumCornersWorldSpace(const glm::mat4& projview)
	{
		const auto inv = glm::inverse(projview);

		std::vector<glm::vec4> frustumCorners;
		for (unsigned int x = 0; x < 2; ++x) {
			for (unsigned int y = 0; y < 2; ++y) {
				for (unsigned int z = 0; z < 2; ++z) {
					const glm::vec4 pt = inv * glm::vec4(2.0f * static_cast<float>(x) - 1.0f, 2.0f * static_cast<float>(y) - 1.0f, 2.0f * static_cast<float>(z) - 1.0f, 1.0f);
					frustumCorners.push_back(pt / pt.w);
				}
			}
		}

		return frustumCorners;
	}

	std::vector<glm::vec4> ShadowMapRenderer::getFrustumCornersWorldSpace(const glm::mat4& proj, const glm::mat4& view)
	{
		return getFrustumCornersWorldSpace(proj * view);
	}

	glm::mat4 ShadowMapRenderer::getLightSpaceMatrix(const float nearPlane, const float farPlane)
	{
		const auto proj    = glm::perspective(glm::radians(GetCamera().m_fov), GetWindow().GetTargetAspectRatio(), nearPlane, farPlane); // m_camera.GetProjectionMatrix(Window::GetTargetAspectRatio());
		const auto corners = getFrustumCornersWorldSpace(proj, GetCamera().GetViewMatrix());

		glm::vec3 center = glm::vec3(0, 0, 0);
		for (const auto& v : corners) {
			center += glm::vec3(v);
		}
		center /= corners.size();

		const auto lightView = glm::lookAt(center + lightDir, center, glm::vec3(0.0f, 1.0f, 0.0f));

		float minX = std::numeric_limits<float>::max();
		float maxX = std::numeric_limits<float>::lowest();
		float minY = std::numeric_limits<float>::max();
		float maxY = std::numeric_limits<float>::lowest();
		float minZ = std::numeric_limits<float>::max();
		float maxZ = std::numeric_limits<float>::lowest();
		for (const auto& v : corners) {
			const auto trf = lightView * v;
			minX           = std::min(minX, trf.x);
			maxX           = std::max(maxX, trf.x);
			minY           = std::min(minY, trf.y);
			maxY           = std::max(maxY, trf.y);
			minZ           = std::min(minZ, trf.z);
			maxZ           = std::max(maxZ, trf.z);
		}

		// Tune this parameter according to the scene
		constexpr float zMult = 5.0f;
		if (minZ < 0) {
			minZ *= zMult;
		}
		else {
			minZ /= zMult;
		}
		if (maxZ < 0) {
			maxZ /= zMult;
		}
		else {
			maxZ *= zMult;
		}

		const glm::mat4 lightProjection = glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);
		return lightProjection * lightView;
	}

	std::vector<glm::mat4> ShadowMapRenderer::getLightSpaceMatrices()
	{
		std::vector<glm::mat4> ret;
		for (size_t i = 0; i < shadowCascadeLevels.size() + 1; ++i) {
			if (i == 0) {
				ret.push_back(getLightSpaceMatrix(CAMERA_NEAR_PLANE, shadowCascadeLevels[i]));
			}
			else if (i < shadowCascadeLevels.size()) {
				ret.push_back(getLightSpaceMatrix(shadowCascadeLevels[i - 1], shadowCascadeLevels[i]));
			}
			else {
				ret.push_back(getLightSpaceMatrix(shadowCascadeLevels[i - 1], CAMERA_FAR_PLANE));
			}
		}
		return ret;
	}


	void ShadowMapRenderer::Initalize()
	{
		if (!m_depthShader.LoadFromFiles("resources/shaders/depth.vert", "resources/shaders/depth.frag", "resources/shaders/depth.geom")) {
			GetDefaultLogger()->error("Failed to load depth shader");
		}


		glGenFramebuffers(1, &lightFBO);

		glGenTextures(1, &lightDepthMaps);
		glBindTexture(GL_TEXTURE_2D_ARRAY, lightDepthMaps);
		glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT32F, depthMapResolution, depthMapResolution, int(shadowCascadeLevels.size()) + 1, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		//		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		//		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

		constexpr float bordercolor[] = {1.0f, 1.0f, 1.0f, 1.0f};
		glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, bordercolor);

		glBindFramebuffer(GL_FRAMEBUFFER, lightFBO);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, lightDepthMaps, 0);
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);

		int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE) {
			GetRenderer().log->error("Framebuffer is not complete!");
			return;
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		glGenBuffers(1, &matricesUBO);
		glBindBuffer(GL_UNIFORM_BUFFER, matricesUBO);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4x4) * 16, nullptr, GL_STATIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, 0, matricesUBO);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}


	void ShadowMapRenderer::RenderShadowMaps()
	{
		m_depthShader.Bind();

		const auto lightMatrices = getLightSpaceMatrices();
		glBindBuffer(GL_UNIFORM_BUFFER, matricesUBO);
		for (size_t i = 0; i < lightMatrices.size(); ++i) {
			glBufferSubData(GL_UNIFORM_BUFFER, static_cast<GLintptr>(i * sizeof(glm::mat4x4)), sizeof(glm::mat4x4), glm::value_ptr(lightMatrices[i]));
		}
		glBindBuffer(GL_UNIFORM_BUFFER, 0);

		glBindFramebuffer(GL_FRAMEBUFFER, lightFBO);
		glViewport(0, 0, depthMapResolution, depthMapResolution);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


		auto view = GetCurrentSceneRegistry().view<Engine::Components::EntityMetadata, Engine::Components::Transform, Engine::Components::ModelRenderer, Engine::Components::ShadowCaster>();

		for (auto [entity, metadata, transform, renderer, shadowCaster] : view.each()) {
			if (!renderer.visible) continue;
			// TODO, not allow invisible but shadow casting?
			// if (!renderer.model.IsValid()) continue;

			auto* model = GetAssetManager().Get(renderer.model);
			if (!model) continue;

			glm::mat4 modelMatrix = CalculateModelMatrix(transform);
			m_depthShader.SetMat4("model", &modelMatrix);
			model->Draw(m_depthShader, true, false);
		}

		// Unbind shadow buffer now
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		// Reset draw size to full screen, not shadow map resolution
		GetWindow().SetFullViewport();
	}

	void ShadowMapRenderer::UploadShadowMatrices(Engine::Shader& shader, glm::mat4& V, int textureSlot)
	{
		shader.Bind();
		ENGINE_GLCheckError();
		shader.SetInt("cascadeCount", static_cast<int>(shadowCascadeLevels.size()));
		shader.SetInt("shadowMap", textureSlot);

		for (size_t i = 0; i < shadowCascadeLevels.size(); ++i) {
			shader.SetFloat("cascadePlaneDistances[" + std::to_string(i) + "]", shadowCascadeLevels[i]);
		}
		ENGINE_GLCheckError();
		shader.SetVec3("lightDir", lightDir);
		ENGINE_GLCheckError();
		shader.SetFloat("farPlane", CAMERA_FAR_PLANE);
		ENGINE_GLCheckError();
		shader.SetVec2("texScale", glm::vec2(1.0, 1.0));
		ENGINE_GLCheckError();
		shader.SetVec3("viewPos", GetCamera().GetPosition());
		ENGINE_GLCheckError();
		shader.SetMat4("view", &V);
		glm::mat4 proj = GetCamera().GetProjectionMatrix();
		shader.SetMat4("projection", &proj);
		ENGINE_GLCheckError();
		glActiveTexture(GL_TEXTURE0 + textureSlot);
		glBindTexture(GL_TEXTURE_2D_ARRAY, lightDepthMaps);
	}
} // namespace Engine

#include "assets/AssetManager.inl"