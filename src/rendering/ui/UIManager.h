#pragma once

#include "Camera.h"
#include "animation/rendering/renderer_impl.h"
#include "core/Entity.h"
#include "sound/SoundManager.h"



#include "core/module/Module.h"
#include "ModelPreview.h"
#include "rendering/ui/windows/AssetUIRenderer.h"
#include "rendering/ui/windows/InspectorRenderer.h"
#include "rendering/ui/windows/MaterialEditor.h"
#include "rendering/effects/ssao/GBuffer.h"

#include <typeindex>

#include <unordered_map>


namespace Engine {
	class GEngine;
	namespace UI {

		class UIManager : public Module {
		  public:
			void                      onInit() override;
			void                      onUpdate(float dt) override;
			void                      onGameStart() override {}
			void                      onShutdown() override;
			[[nodiscard]] std::string name() const override { return "UIModule"; };
            void                      setLuaBindings() override;

			void BeginDockspace(float ht);
			void EndDockspace();

			std::shared_ptr<Texture> m_audioIconTexture;
			std::shared_ptr<Texture> m_terrainIconTexture;
			std::shared_ptr<Texture> m_animationIconTexture;
			std::shared_ptr<Texture> m_folderIconTexture;
			std::shared_ptr<Texture> m_fileIconTexture;
			std::shared_ptr<Texture> m_modelIconTexture;
			std::shared_ptr<Texture> m_shaderIconTexture;
			std::shared_ptr<Texture> m_particleIconTexture;
			std::shared_ptr<Texture> m_materialIconTexture;

			AssetHandle<Material>    m_selectedMaterial;
			AssetHandle<Engine::Rendering::Model>    m_selectedModel;

			// Selected entity
			Entity m_selectedEntity = Entity();

			std::unique_ptr<InspectorRenderer> m_inspectorRenderer;
			bool                               isOverSceneView() const;
            [[nodiscard]] const Engine::Entity& getSelectedEntity() const { return m_selectedEntity; }
		  private:
			// UI rendering methods
			void  RenderHierarchyWindow();
			float RenderTopBar(float top);
			float RenderMainMenuBar();
			void  RenderPauseOverlay();
            void RenderGBufferDebug(std::shared_ptr<GBuffer> gbuffer);
            void RenderModelDebug(AssetHandle<Engine::Rendering::Model> handle);

			bool                             m_overSceneView = false;
			std::unique_ptr<AssetUIRenderer> m_uiAssetRenderer;
			std::unique_ptr<MaterialEditor>  m_materialEditor;
			int                              m_selectedTheme = 0;
			void                             RenderEntityTreeNode(Entity entity);

        };
	} // namespace UI
} // namespace Engine