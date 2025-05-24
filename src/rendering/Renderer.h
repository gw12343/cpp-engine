#pragma once

#include "Camera.h"
#include "Model.h"
#include "Shader.h"
#include "Skybox.h"
#include "Texture.h"
#include "core/Window.h"

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
		void RenderEntities(entt::registry& registry) const;
		void RenderSkybox();

		const Shader& GetShader() const { return m_shader; }

		// Get default textures
		std::shared_ptr<Texture> GetDefaultWhiteTexture() const { return m_defaultWhiteTexture; }
		std::shared_ptr<Texture> GetDefaultNormalTexture() const { return m_defaultNormalTexture; }
		std::shared_ptr<Texture> GetDefaultBlackTexture() const { return m_defaultBlackTexture; }

	  private:
		// Create default textures
		bool CreateDefaultTextures();

		Engine::Window&         m_window;
		Engine::Camera&         m_camera;
		Engine::Shader          m_shader;
		Engine::Shader          m_skyboxShader;
		std::unique_ptr<Skybox> m_skybox;

		// Default textures
		std::shared_ptr<Texture> m_defaultWhiteTexture;
		std::shared_ptr<Texture> m_defaultNormalTexture;
		std::shared_ptr<Texture> m_defaultBlackTexture;
	};
} // namespace Engine