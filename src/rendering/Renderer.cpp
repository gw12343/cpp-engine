#include "Renderer.h"

#include "glad/glad.h"
#include "assets/impl/ModelLoader.h"
#include "utils/Utils.h"
#include "core/EngineData.h"
#include "core/Input.h"
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
#include <RmlUi/Core.h>
#include "rendering/ui/RmlUiBackend.h"
#include "rendering/ui/GameUIManager.h"
#include "scripting/ScriptManager.h"
#include "DebugGroup.h"


namespace Engine {

	void Renderer::onInit()
	{
		m_shadowRenderer = std::make_shared<ShadowMapRenderer>();

        ReloadShaders();

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
        InitFullscreenQuad();
	}

    void Renderer::InitFullscreenQuad()
    {
        if (quadVAO != 0) return;

        float quadVertices[] = {
                // positions   // tex
                -1.0f, -1.0f,  0.0f, 0.0f,
                1.0f, -1.0f,  1.0f, 0.0f,
                1.0f,  1.0f,  1.0f, 1.0f,

                -1.0f, -1.0f,  0.0f, 0.0f,
                1.0f,  1.0f,  1.0f, 1.0f,
                -1.0f,  1.0f,  0.0f, 1.0f
        };

        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);

        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                              (void*)(2 * sizeof(float)));

        glBindVertexArray(0);
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

    void Renderer::RenderLightingPass()
    {
        ZoneScopedN("Deferred Lighting");

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);

        m_lightingShader.Bind();

        // G-buffer textures
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, GetWindow().GetGBuffer()->GetDepth());
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, GetWindow().GetGBuffer()->GetNormal());
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, GetWindow().GetGBuffer()->GetAlbedo());
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, GetWindow().GetGBuffer()->GetMaterial());



        glm::mat4 V = GetCamera().GetViewMatrix();
        glm::mat4 viewInv = glm::inverse(V);
        m_lightingShader.SetMat4("invView", &viewInv);
        glm::mat4 projInv = glm::inverse(GetCamera().GetProjectionMatrix());
        m_lightingShader.SetMat4("invProjection", &projInv);


        m_lightingShader.SetInt("gDepth", 0);
        m_lightingShader.SetInt("gNormal", 1);
        m_lightingShader.SetInt("gAlbedo", 2);
        m_lightingShader.SetInt("gMaterial", 3);
        m_lightingShader.SetInt("skybox", 4);

        m_skybox->m_texture->Bind(4);

        // Camera + light uniforms
        m_shadowRenderer->UploadShadowMatrices(m_lightingShader, V, 5);


        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        glEnable(GL_DEPTH_TEST);
    }


    void Renderer::onUpdate(float dt)
	{
		ZoneScopedN("Render");
		PreRender();
		{
			ZoneScopedN("Render Shadow Maps");
			RenderShadowMaps();
		}

        {
            ZoneScopedN("Render GBuffer");
            DebugGroup group("Deferred GBuffer Pass");



            GetWindow().GetGBuffer()->Bind();


            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            RenderEntitiesGBuffer();
            GetTerrainManager().RenderGBuffer();

            GetWindow().GetGBuffer()->Unbind();
        }

#ifndef GAME_BUILD
		Engine::Window::GetFramebuffer(Window::FramebufferID::GAME_OUT)->Bind();
#endif

        {
			ZoneScopedN("Render Lighting Pass");
            DebugGroup group("Deferred Lighting Pass");
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glDisable(GL_CULL_FACE);
            RenderLightingPass();

        }
//		{
//			ZoneScopedN("Render Animations");
//			GetAnimationManager().Render();
//		}

//		{
//			ZoneScopedN("Render Particles");
//			GetParticleManager().Render();
//		}
		{
			ZoneScopedN("Render RmlUi");
			// Render RmlUi into the framebuffer
			GetGameUIManager().Render();
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

#ifndef GAME_BUILD
        GetScriptManager().EditorScriptUpdate(dt);
#endif




		Engine::Framebuffer::Unbind();
		{
			ZoneScopedN("Post Render");
			PostRender();
		}
	}


    void Renderer::RenderEntitiesGBuffer()
    {
        ZoneScopedN("Render Entities GBuffer");

        glDisable(GL_CULL_FACE);          // match forward for now
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);

        ENGINE_GLCheckError();

        Shader& gbufferShader = GetGBufferShader();

        gbufferShader.Bind();

        // View / projection matricies

        glm::mat4 V = GetCamera().GetViewMatrix();
        gbufferShader.SetMat4("view", &V);
        glm::mat4 proj = GetCamera().GetProjectionMatrix();
        gbufferShader.SetMat4("projection", &proj);

        ENGINE_GLCheckError();

        auto view = GetCurrentSceneRegistry().view<
                Engine::Components::EntityMetadata,
                Engine::Components::Transform,
                Engine::Components::ModelRenderer
        >();

        for (auto [entity, metadata, transform, renderer] : view.each()) {
            if (!renderer.visible)
                continue;

            renderer.Draw(gbufferShader, transform, true);
        }

        //TODO add unbind??
        //gbufferShader.Unbind();
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
				const ozz::math::Float4x4 transform              = FromMatrix(e.GetComponent<Components::Transform>().GetWorldMatrix());

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

	void Renderer::RenderShadowMaps()
	{
		m_shadowRenderer->RenderShadowMaps();
	}
	void Renderer::RenderGizmos(bool mousePicking)
	{
		auto view = GetCurrentSceneRegistry().view<Engine::Components::EntityMetadata, Engine::Components::Transform, Engine::Components::GizmoComponent>();
		for (auto [entity, metadata, transform, gizmo] : view.each()) {
			const ozz::math::Float4x4 t = FromMatrix(transform.GetWorldMatrix());
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

    void Renderer::ReloadShaders()
	{
		log->info("Reloading shaders...");
		if (!m_shader.LoadFromFiles("resources/shaders/vert.glsl", "resources/shaders/frag.glsl", std::nullopt)) {
			log->error("Failed to load default shader");
		}

		if (!m_mousePickingShader.LoadFromFiles("resources/shaders/picking.vert", "resources/shaders/picking.frag", std::nullopt)) {
			log->error("Failed to load mouse picking shader");
		}

		// Load model preview shader
		if (!m_modelPreviewShader.LoadFromFiles("resources/shaders/preview_vert.glsl", "resources/shaders/preview_frag.glsl", std::nullopt)) {
			log->error("Failed to load model preview shader");
		}

		// Load material preview shader
		if (!m_materialPreviewShader.LoadFromFiles("resources/shaders/material_preview_vert.glsl", "resources/shaders/material_preview_frag.glsl", std::nullopt)) {
			log->error("Failed to load material preview shader");
		}

        // Load GBuffer shader
        if (!m_gbufferShader.LoadFromFiles("resources/shaders/gbuffer_vert.glsl", "resources/shaders/gbuffer_frag.glsl", std::nullopt)) {
            log->error("Failed to load gbuffer shader");
            return;
        }

        // Load lighting shader
        if (!m_lightingShader.LoadFromFiles("resources/shaders/lighting_vert.glsl", "resources/shaders/lighting_frag.glsl", std::nullopt)) {
            log->error("Failed to load lighting shader");
            return;
        }
	}

    std::shared_ptr<ShadowMapRenderer> Renderer::GetShadowRenderer()
    {
        return m_shadowRenderer;
    }

} // namespace Engine