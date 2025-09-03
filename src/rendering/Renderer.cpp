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
#include <spdlog/spdlog.h>

namespace Engine {

	unsigned int quadVAO = 0;
	unsigned int quadVBO;

	void InitQuad()
	{
		float quadVertices[] = {// positions   // texCoords
		                        -1.0f, 1.0f, 0.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, -1.0f, 1.0f, 0.0f,

		                        -1.0f, 1.0f, 0.0f, 1.0f, 1.0f,  -1.0f, 1.0f, 0.0f, 1.0f, 1.0f,  1.0f, 1.0f};
		glGenVertexArrays(1, &quadVAO);
		glGenBuffers(1, &quadVBO);
		glBindVertexArray(quadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*) 0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*) (2 * sizeof(float)));
	}


	void Renderer::onInit()
	{
		m_shadowRenderer = std::make_shared<ShadowMapRenderer>();

		//		if (!m_shader.LoadFromFiles("resources/shaders/vert.glsl", "resources/shaders/frag.glsl", std::nullopt)) {
		//			log->error("Failed to load default shader");
		//			return;
		//		}

		if (!m_gbufferShader.LoadFromFiles("resources/shaders/gbuffer_model.vert", "resources/shaders/gbuffer_model.frag", std::nullopt)) {
			log->error("Failed to load gbuffer shader");
			return;
		}

		if (!m_lightingPassShader.LoadFromFiles("resources/shaders/lighting_pass.vert", "resources/shaders/lighting_pass.frag", std::nullopt)) {
			log->error("Failed to load lighting pass shader");
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
		InitQuad();
	}

	void Renderer::PreRender()
	{
		// Clear the screen
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Bind our shader
		m_gbufferShader.Bind();

		// Set up view and projection matrices
		glm::mat4 view       = GetCamera().GetViewMatrix();
		glm::mat4 projection = GetCamera().GetProjectionMatrix();

		m_gbufferShader.SetMat4("view", &view);
		m_gbufferShader.SetMat4("projection", &projection);
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
		// Use the deferred lighting shader
		m_lightingPassShader.Bind();

		// Set uniforms
		m_lightingPassShader.SetVec3("viewPos", GetCamera().GetPosition());
		m_lightingPassShader.SetVec3("lightDir", glm::vec3(-0.2f, -1.0f, -0.3f));
		m_lightingPassShader.SetVec3("lightColor", glm::vec3(1.0f));

		m_lightingPassShader.SetInt("gPosition", 0);
		m_lightingPassShader.SetInt("gNormal", 1);
		m_lightingPassShader.SetInt("gAlbedoSpec", 2);

		// Draw quad
		glBindVertexArray(quadVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);
	}

	void Renderer::onUpdate(float dt)
	{
		PreRender();

		// --------------------
		// 1️⃣ GEOMETRY PASS -> G-BUFFER
		// --------------------
		auto gbuffer = Engine::Window::GetFramebuffer(Window::FramebufferID::GBUFFER);
		gbuffer->Bind();


		glClearColor(0.0, 0.0, 0.0, 1.0); // keep it black so it doesn't leak into g-buffer
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		m_gbufferShader.Bind();

		RenderEntities(); // writes geometry to G-buffer
		Engine::Framebuffer::Unbind();


		// --------------------
		// 2️⃣ SKYBOX PASS -> GAME_OUT
		// --------------------
		auto gameOut = Engine::Window::GetFramebuffer(Window::FramebufferID::GAME_OUT);
		gameOut->Bind();

		// Clear color buffer; depth will be used for skybox
		// glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


		// --------------------
		// 3️⃣ LIGHTING PASS -> GAME_OUT
		// --------------------
		// Bind G-buffer textures for lighting shader
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, gbuffer->GetColorAttachment(0)); // gPosition
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, gbuffer->GetColorAttachment(1)); // gNormal
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, gbuffer->GetColorAttachment(2)); // gAlbedoSpec

		RenderLightingPass(); // fullscreen quad; skips empty pixels


		glBindFramebuffer(GL_READ_FRAMEBUFFER, gbuffer->GetFBO());
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, gameOut->GetFBO()); // write to default framebuffer
		glBlitFramebuffer(0, 0, GetWindow().GetWidth(), GetWindow().GetHeight(), 0, 0, GetWindow().GetWidth(), GetWindow().GetHeight(), GL_DEPTH_BUFFER_BIT, GL_NEAREST);


		RenderSkybox();

		Engine::Framebuffer::Unbind();


		// --------------------
		// 4️⃣ POST-PROCESS
		// --------------------
		PostRender();
	}


	void Renderer::RenderEntities()
	{
		glDisable(GL_CULL_FACE);
		ENGINE_GLCheckError();
		glm::mat4 V = GetCamera().GetViewMatrix();
		// m_shadowRenderer->UploadShadowMatrices(m_gbufferShader, V, 3);
		ENGINE_GLCheckError();
		// Create a view for entities with Transform and ModelRenderer components
		auto view = GetCurrentSceneRegistry().view<Engine::Components::EntityMetadata, Engine::Components::Transform, Engine::Components::ModelRenderer>();
		for (auto [entity, metadata, transform, renderer] : view.each()) {
			if (!renderer.visible) continue;
			// Draw model
			renderer.Draw(m_gbufferShader, transform, true);
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


} // namespace Engine