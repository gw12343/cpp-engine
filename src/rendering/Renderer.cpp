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

	void Renderer::onInit()
	{
		m_shadowRenderer = std::make_shared<ShadowMapRenderer>();

		if (!m_shader.LoadFromFiles("resources/shaders/vert.glsl", "resources/shaders/frag.glsl", std::nullopt)) {
			log->error("Failed to load default shader");
			return;
		}

		// Load skybox shader
		if (!m_skyboxShader.LoadFromFiles("resources/shaders/skybox_vert.glsl", "resources/shaders/skybox_frag.glsl", std::nullopt)) {
			log->error("Failed to load skybox shader");
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
		PreRender();
		RenderShadowMaps();
		Engine::Window::GetFramebuffer(Window::FramebufferID::GAME_OUT)->Bind();
		PreRender();
		GetAnimationManager().Render();
		RenderEntities();
		GetTerrainManager().Render();
		RenderSkybox();
		GetParticleManager().Render();
		Engine::Framebuffer::Unbind();
		PostRender();
	}


	void Renderer::RenderEntities()
	{
		glDisable(GL_CULL_FACE);
		ENGINE_GLCheckError();
		glm::mat4 V = GetCamera().GetViewMatrix();
		m_shadowRenderer->UploadShadowMatrices(m_shader, V);
		ENGINE_GLCheckError();
		// Create a view for entities with Transform and ModelRenderer components
		auto view = GetRegistry().view<Engine::Components::EntityMetadata, Engine::Components::Transform, Engine::Components::ModelRenderer>();
		for (auto [entity, metadata, transform, renderer] : view.each()) {
			if (!renderer.visible) continue;
			// Draw model
			renderer.Draw(GetShader(), transform);
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