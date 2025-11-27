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
		void                      onGameStart() override {}
		void                      onShutdown() override;
		void                      ReloadShaders();
		[[nodiscard]] std::string name() const override { return "RendererModule"; };


		void PreRender();
		void PostRender();

		void RenderEntitiesMousePicking();
		void RenderEntities();
		void RenderShadowMaps();
		void RenderSkybox();

		Shader& GetShader() { return m_shader; }
		Shader& GetMousePickingShader() { return m_mousePickingShader; }
		Shader& GetModelPreviewShader() { return m_modelPreviewShader; }
		Shader& GetMaterialPreviewShader() { return m_materialPreviewShader; }
		Shader& GetTerrainShader() { return m_terrainShader; }

		std::shared_ptr<ShadowMapRenderer> GetShadowRenderer();

	  private:
		std::shared_ptr<ShadowMapRenderer> m_shadowRenderer;

		Engine::Shader          m_shader;
		Engine::Shader          m_mousePickingShader;
		Engine::Shader          m_modelPreviewShader;
		Engine::Shader          m_materialPreviewShader;
		Engine::Shader          m_terrainShader;
		Engine::Shader          m_skyboxShader;
		std::unique_ptr<Skybox> m_skybox;
		void                    RenderGizmos(bool mousePicking);
	};
} // namespace Engine