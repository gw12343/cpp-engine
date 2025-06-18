#pragma once

#include "Camera.h"
#include "Model.h"
#include "Shader.h"
#include "Skybox.h"
#include "Texture.h"
#include "core/Window.h"
#include "rendering/shadows/ShadowMapRenderer.h"

#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <memory>

namespace Engine {
	class Renderer {
	  public:
		Renderer(Engine::Window& window, Engine::Camera& camera);
		~Renderer();

		bool Initialize();
		void PreRender();
		void PostRender();
		void Shutdown();
		// void UploadShadowMatrices(Engine::Shader& shader, glm::mat4& V);
		void RenderEntities(entt::registry& registry);
		void RenderShadowMaps(entt::registry& registry);
		void RenderSkybox();

		const Shader& GetShader() const { return m_shader; }

	  private:
		std::unique_ptr<ShadowMapRenderer> m_shadowRenderer;

		Engine::Window&         m_window;
		Engine::Camera&         m_camera;
		Engine::Shader          m_shader;
		Engine::Shader          m_skyboxShader;
		std::unique_ptr<Skybox> m_skybox;
	};
} // namespace Engine