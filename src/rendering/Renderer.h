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
	class Renderer : public Module {
	  public:
		void                      onInit() override;
		void                      onUpdate(float dt) override;
		void                      onShutdown() override;
		[[nodiscard]] std::string name() const override { return "RendererModule"; };


		void PreRender();
		void PostRender();

		void UploadShadowMatrices(Engine::Shader& shader, glm::mat4& V);
		void RenderEntities();
		void RenderShadowMaps();
		void RenderSkybox();

		Shader& GetShader() { return m_shader; }

	  private:
		std::unique_ptr<ShadowMapRenderer> m_shadowRenderer;

		Engine::Shader          m_shader;
		Engine::Shader          m_skyboxShader;
		std::unique_ptr<Skybox> m_skybox;
	};
} // namespace Engine