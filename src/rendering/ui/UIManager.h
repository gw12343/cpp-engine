#pragma once

#include "Camera.h"
#include "animation/renderer_impl.h"
#include "core/Entity.h"
#include "sound/SoundManager.h"

#include "entt/entt.hpp"
#include "imgui.h"
#include "core/module/Module.h"
#include "ModelPreview.h"
#include "rendering/ui/windows/AssetUIRenderer.h"
#include "rendering/ui/windows/InspectorRenderer.h"
#include "rendering/ui/windows/MaterialEditor.h"
#include <memory>
#include <typeindex>
#include <string>
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

			void BeginDockspace(float ht);
			void EndDockspace();

			std::shared_ptr<Texture> m_audioIconTexture;
			std::shared_ptr<Texture> m_terrainIconTexture;
			std::shared_ptr<Texture> m_animationIconTexture;
			AssetHandle<Material>    m_selectedMaterial;

			// Selected entity
			Entity m_selectedEntity;

			std::unique_ptr<InspectorRenderer> m_inspectorRenderer;
			bool                               isOverSceneView() const;

		  private:
			// UI rendering methods
			void  RenderHierarchyWindow();
			float RenderTopBar(float top);
			float RenderMainMenuBar();
			void  RenderPauseOverlay();

			bool                             m_overSceneView = false;
			std::unique_ptr<AssetUIRenderer> m_uiAssetRenderer;
			std::unique_ptr<MaterialEditor>  m_materialEditor;
			int                              m_selectedTheme = 0;
			void                             RenderEntityTreeNode(Entity entity);
		};
	} // namespace UI
} // namespace Engine