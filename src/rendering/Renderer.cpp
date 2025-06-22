#include "Renderer.h"

#include "components/Components.h"
#include "utils/ModelLoader.h"
#include "utils/Utils.h"

#include <spdlog/spdlog.h>

namespace Engine {

	Renderer::Renderer(Engine::Window& window, Engine::Camera& camera) : m_window(window), m_camera(camera)
	{
		m_shadowRenderer = std::make_unique<ShadowMapRenderer>(window, camera);
	}

	Renderer::~Renderer()
	{
		Shutdown();
	}


	bool Renderer::Initialize()
	{
		if (!m_shader.LoadFromFiles("/home/gabe/CLionProjects/cpp-engine/resources/shaders/vert.glsl", "/home/gabe/CLionProjects/cpp-engine/resources/shaders/frag.glsl", std::nullopt)) {
			return false;
		}

		// Load skybox shader
		if (!m_skyboxShader.LoadFromFiles("/home/gabe/CLionProjects/cpp-engine/resources/shaders/skybox_vert.glsl", "/home/gabe/CLionProjects/cpp-engine/resources/shaders/skybox_frag.glsl", std::nullopt)) {
			return false;
		}


		m_skybox            = std::make_unique<Skybox>();
		const std::string p = "/home/gabe/CLionProjects/cpp-engine/resources/textures/output.hdr";
		if (!m_skybox->LoadFromFile(p)) {
			return false;
		}

		// Enable depth testing so closer fragments obscure farther ones
		glEnable(GL_DEPTH_TEST);
		// glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);

		// todo init
		m_shadowRenderer->Initalize();

		return true;
	}

	void Renderer::PreRender()
	{
		// Clear the screen
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Bind our shader
		m_shader.Bind();

		// Set up view and projection matrices
		glm::mat4 view       = m_camera.GetViewMatrix();
		glm::mat4 projection = m_camera.GetProjectionMatrix(Engine::Window::GetTargetAspectRatio());

		m_shader.SetMat4("view", &view);
		m_shader.SetMat4("projection", &projection);
	}

	void Renderer::PostRender()
	{
		m_window.SwapBuffers();
	}

	void Renderer::Shutdown()
	{
		// Clean up any renderer-specific resources
	}


	void Renderer::RenderEntities(entt::registry& registry)
	{
		glDisable(GL_CULL_FACE);
		ENGINE_GLCheckError();
		glm::mat4 V = m_camera.GetViewMatrix();
		// UploadShadowMatrices(m_shader, V);
		m_shadowRenderer->UploadShadowMatrices(m_shader, V);
		ENGINE_GLCheckError();
		// Create a view for entities with Transform and ModelRenderer components
		auto view = registry.view<Engine::Components::EntityMetadata, Engine::Components::Transform, Engine::Components::ModelRenderer>();
		for (auto [entity, metadata, transform, renderer] : view.each()) {
			// Draw model
			renderer.Draw(GetShader(), transform);
		}
	}

	void Renderer::RenderSkybox()
	{
		m_skyboxShader.Bind();

		// Set up view and projection matrices for skybox
		glm::mat4 view       = m_camera.GetViewMatrix();
		glm::mat4 projection = m_camera.GetProjectionMatrix(Engine::Window::GetTargetAspectRatio());

		m_skyboxShader.SetMat4("view", &view);
		m_skyboxShader.SetMat4("projection", &projection);

		// Draw skybox
		m_skybox->Draw(m_skyboxShader);
	}
	void Renderer::RenderShadowMaps(entt::registry& registry)
	{
		m_shadowRenderer->RenderShadowMaps(registry);
	}


} // namespace Engine