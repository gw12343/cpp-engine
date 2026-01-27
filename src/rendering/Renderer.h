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

        void InitFullscreenQuad();
		void PreRender();
		static void PostRender();

        void RenderLightingPass();
		void RenderEntitiesMousePicking();
		void RenderEntities();
		void RenderEntitiesGBuffer();
		void RenderShadowMaps();
		void RenderSkybox();

		Shader& GetShader() { return m_shader; }
		Shader& GetLightingShader() { return m_lightingShader; }
		Shader& GetGBufferShader() { return m_gbufferShader; }
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
		Engine::Shader          m_gbufferShader;
		Engine::Shader          m_lightingShader;


		std::unique_ptr<Skybox> m_skybox;
        GLuint quadVAO = 0;
        GLuint quadVBO = 0;

		static void                    RenderGizmos(bool mousePicking);
	};
} // namespace Engine