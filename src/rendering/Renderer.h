#pragma once

#include "Camera.h"
#include "Model.h"
#include "Shader.h"
#include "Skybox.h"
#include "core/Window.h"
#include "rendering/shadows/ShadowMapRenderer.h"
#include "rendering/effects/bloom/BloomRenderer.h"
#include "rendering/text/Text3DRenderer.h"







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
        void RenderBloomPass();
		void RenderText3D();
		void RenderEntitiesMousePicking();
		void RenderEntitiesGBuffer();
		void RenderSSAO();
		void RenderSSAOBlur();
		void RenderShadowMaps();

		Shader& GetShader() { return m_shader; }
		Shader& GetLightingShader() { return m_lightingShader; }
		Shader& GetGBufferShader() { return m_gbufferShader; }
		Shader& GetMousePickingShader() { return m_mousePickingShader; }
		Shader& GetModelPreviewShader() { return m_modelPreviewShader; }
		Shader& GetMaterialPreviewShader() { return m_materialPreviewShader; }
		Shader& GetTerrainShader() { return m_terrainShader; }

		std::shared_ptr<ShadowMapRenderer> GetShadowRenderer();
		std::shared_ptr<BloomRenderer> GetBloomRenderer() { return m_bloomRenderer; }

        GLuint quadVAO = 0;
    private:
		std::shared_ptr<ShadowMapRenderer> m_shadowRenderer;
		std::shared_ptr<BloomRenderer> m_bloomRenderer;
		std::unique_ptr<Text3DRenderer> m_text3DRenderer;

		Engine::Shader          m_shader;
		Engine::Shader          m_mousePickingShader;
		Engine::Shader          m_modelPreviewShader;
		Engine::Shader          m_materialPreviewShader;
		Engine::Shader          m_terrainShader;
		Engine::Shader          m_gbufferShader;
		Engine::Shader          m_lightingShader;

        Engine::Shader          m_ssaoShader;
        Engine::Shader          m_ssaoBlurShader;


		std::unique_ptr<Skybox> m_skybox;
        GLuint quadVBO = 0;

		static void                    RenderGizmos(bool mousePicking);
	};
} // namespace Engine