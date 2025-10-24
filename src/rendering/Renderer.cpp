#include "Renderer.h"

#include "assets/impl/ModelLoader.h"
#include "utils/Utils.h"
#include "core/EngineData.h"
#include "terrain/TerrainManager.h"
#include "animation/AnimationManager.h"
#include "rendering/particles/ParticleManager.h"
#include "components/impl/EntityMetadataComponent.h"
#include "components/impl/TransformComponent.h"
#include "components/impl/ModelRendererComponent.h"
#include "components/impl/SkinnedMeshComponent.h"
#include "components/impl/AnimationPoseComponent.h"
#include "components/impl/GizmoComponent.h"
#include <spdlog/spdlog.h>
#include <tracy/Tracy.hpp>

namespace Engine {

	void Renderer::onInit()
	{
		m_shadowRenderer = std::make_shared<ShadowMapRenderer>();

		if (!m_shader.LoadFromFiles("resources/shaders/vert.glsl", "resources/shaders/frag.glsl", std::nullopt)) {
			log->error("Failed to load default shader");
			return;
		}

		if (!m_mousePickingShader.LoadFromFiles("resources/shaders/picking.vert", "resources/shaders/picking.frag", std::nullopt)) {
			log->error("Failed to load mouse picking shader");
			return;
		}

		// Load skybox shader
		if (!m_skyboxShader.LoadFromFiles("resources/shaders/skybox_vert.glsl", "resources/shaders/skybox_frag.glsl", std::nullopt)) {
			log->error("Failed to load skybox shader");
			return;
		}

		// Load model preview shader
		if (!m_modelPreviewShader.LoadFromFiles("resources/shaders/preview_vert.glsl", "resources/shaders/preview_frag.glsl", std::nullopt)) {
			log->error("Failed to load model preview shader");
			return;
		}


		m_skybox            = std::make_unique<Skybox>();
		const std::string p = "resources/textures/output.hdr";
		if (!m_skybox->LoadFromFile(p)) {
			log->error("Failed to load skybox");
			return;
		}

		// Enable depth testing so closer fragments obscure farther ones
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);

		m_shadowRenderer->Initalize();
	}

	void Renderer::PreRender()
	{
		// Clear the screen
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Bind our shader
		m_shader.Bind();

		// Set up view and projection matrices
		glm::mat4 view       = GetCamera().GetViewMatrix();
		glm::mat4 projection = GetCamera().GetProjectionMatrix();

		m_shader.SetMat4("view", &view);
		m_shader.SetMat4("projection", &projection);
	}

	void Renderer::PostRender()
	{
		GetWindow().SwapBuffers();
	}

	void Renderer::onShutdown()
	{
	}

	void Renderer::onUpdate(float dt)
	{
		ZoneScopedN("Render");
		PreRender();
		{
			ZoneScopedN("Render Shadow Maps");
			RenderShadowMaps();
		}
#ifndef GAME_BUILD
		Engine::Window::GetFramebuffer(Window::FramebufferID::GAME_OUT)->Bind();
#endif
		PreRender();
		{
			ZoneScopedN("Render Animations");
			GetAnimationManager().Render();
		}
		{
			ZoneScopedN("Render Entities");
			RenderEntities();
		}
		{
			ZoneScopedN("Render Terrains");
			GetTerrainManager().Render();
		}
		{
			ZoneScopedN("Render Skybox");
			RenderSkybox();
		}
		{
			ZoneScopedN("Render Particles");
			GetParticleManager().Render();
		}
#ifndef GAME_BUILD
		{
			ZoneScopedN("Render Gizmos");
			if (GetState() == EDITOR || GetState() == PAUSED) {
				RenderGizmos(false);
			}
		}
		{
			ZoneScopedN("Render Mouse Picking");
			Engine::Window::GetFramebuffer(Window::FramebufferID::MOUSE_PICKING)->Bind();
			RenderEntitiesMousePicking();
		}
#endif
		Engine::Framebuffer::Unbind();
		{
			ZoneScopedN("Post Render");
			PostRender();
		}
	}


	void Renderer::RenderEntities()
	{
		glDisable(GL_CULL_FACE);
		ENGINE_GLCheckError();
		glm::mat4 V = GetCamera().GetViewMatrix();
		m_shadowRenderer->UploadShadowMatrices(m_shader, V, 3);
		ENGINE_GLCheckError();
		// Create a view for entities with Transform and ModelRenderer components
		auto view = GetCurrentSceneRegistry().view<Engine::Components::EntityMetadata, Engine::Components::Transform, Engine::Components::ModelRenderer>();
		for (auto [entity, metadata, transform, renderer] : view.each()) {
			if (!renderer.visible) continue;
			// Draw model
			renderer.Draw(GetShader(), transform, true);
		}
	}

	glm::vec3 EncodeEntityID(entt::entity entityID)
	{
		auto  id = static_cast<uint32_t>(entityID);
		float r  = (float) (id & 0xFF) / 255.0f;
		float g  = (float) ((id >> 8) & 0xFF) / 255.0f;
		float b  = (float) ((id >> 16) & 0xFF) / 255.0f;
		return {r, g, b};
	}

	void Renderer::RenderEntitiesMousePicking()
	{
		glDisable(GL_CULL_FACE);

		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		GetMousePickingShader().Bind();

		ENGINE_GLCheckError();
		glm::mat4 V = GetCamera().GetViewMatrix();
		GetMousePickingShader().SetMat4("view", &V);
		glm::mat4 proj = GetCamera().GetProjectionMatrix();
		GetMousePickingShader().SetMat4("projection", &proj);


		ENGINE_GLCheckError();
		{
			ZoneScopedN("Model Renderer Mouse Picking");
			// Create a view for entities with Transform and ModelRenderer components

			auto view = GetCurrentSceneRegistry().view<Engine::Components::EntityMetadata, Engine::Components::Transform, Engine::Components::ModelRenderer>();
			for (auto [entity, metadata, transform, renderer] : view.each()) {
				glm::vec3 encodedColor = EncodeEntityID(entity);
				GetMousePickingShader().SetVec3("entityIDColor", encodedColor);
				if (!renderer.visible) continue;
				// Draw model
				renderer.Draw(GetMousePickingShader(), transform, false);
			}
		}

		{
			ZoneScopedN("Skinned Mesh Mouse Picking");
			auto view = GetCurrentSceneRegistry().view<Components::SkinnedMeshComponent, Components::Transform>();
			for (auto entity : view) {
				Entity                    e(entity, GetCurrentScene());
				auto&                     skinnedMeshComponent   = e.GetComponent<Components::SkinnedMeshComponent>();
				auto&                     animationPoseComponent = e.GetComponent<Components::AnimationPoseComponent>();
				const ozz::math::Float4x4 transform              = FromMatrix(e.GetComponent<Components::Transform>().worldMatrix);

				glm::vec3 encodedColor = EncodeEntityID(entity);

				// Render each mesh
				for (const Engine::Mesh& mesh : *skinnedMeshComponent.meshes) {
					// Render the mesh

					// Builds skinning matrices, based on the output of the animation stage
					// The mesh might not use (aka be skinned by) all skeleton joints. We
					// use the joint remapping table (available from the mesh object) to
					// reorder model-space matrices and build skinning ones
					for (size_t i = 0; i < mesh.joint_remaps.size(); ++i) {
						(*skinnedMeshComponent.skinning_matrices)[i] = (*animationPoseComponent.model_pose)[mesh.joint_remaps[i]] * mesh.inverse_bind_poses[i];
					}
					GetAnimationManager().renderer_->DrawSkinnedMeshMousePicking(encodedColor, mesh, ozz::make_span(*skinnedMeshComponent.skinning_matrices), transform);
				}
			}
		}
		{
			ZoneScopedN("Gizmo Mouse Picking");
			RenderGizmos(true);
		}
	}

	void Renderer::RenderSkybox()
	{
		m_skyboxShader.Bind();

		// Set up view and projection matrices for skybox
		glm::mat4 view       = GetCamera().GetViewMatrix();
		glm::mat4 projection = GetCamera().GetProjectionMatrix();

		m_skyboxShader.SetMat4("view", &view);
		m_skyboxShader.SetMat4("projection", &projection);

		// Draw skybox
		m_skybox->Draw(m_skyboxShader);
	}
	void Renderer::RenderShadowMaps()
	{
		m_shadowRenderer->RenderShadowMaps();
	}
	std::shared_ptr<ShadowMapRenderer> Renderer::GetShadowRenderer()
	{
		return m_shadowRenderer;
	}
	void Renderer::RenderGizmos(bool mousePicking)
	{
		auto view = GetCurrentSceneRegistry().view<Engine::Components::EntityMetadata, Engine::Components::Transform, Engine::Components::GizmoComponent>();
		for (auto [entity, metadata, transform, gizmo] : view.each()) {
			const ozz::math::Float4x4 t = FromMatrix(transform.worldMatrix);
			// Draw gizmo

			Color color = kBlack;
			if (mousePicking) {
				glm::vec3 encodedColor = EncodeEntityID(entity);
				color.r                = encodedColor.r;
				color.g                = encodedColor.g;
				color.b                = encodedColor.b;
			}
			else {
				color.r = gizmo.color.r;
				color.g = gizmo.color.g;
				color.b = gizmo.color.b;
			}
			GetAnimationManager().renderer_->DrawSphereIm(gizmo.radius, t, color);
		}
	}


} // namespace Engine