#pragma once

#include "Camera.h"
#include "animation/renderer_impl.h"
#include "core/Entity.h"
#include "sound/SoundManager.h"

#include "entt/entt.hpp"
#include "imgui.h"
#include "core/module/Module.h"
#include "ModelPreview.h"
#include "AssetUIRenderer.h"
#include "InspectorRenderer.h"
#include "MaterialEditor.h"
#include <memory>
#include <typeindex>
#include <string>
#include <unordered_map>


namespace Engine {
	class GEngine;
	namespace UI {

		class UIManager : public Module {
		  public:
			void        onInit() override;
			void        onUpdate(float dt) override;
			void        onShutdown() override;
			std::string name() const override { return "UIModule"; };


			void DrawMenuBar();
			void BeginDockspace();
			void EndDockspace();


			std::shared_ptr<Texture> audioIconTexture;
			std::shared_ptr<Texture> terrainIconTexture;
			AssetHandle<Material>    selectedMaterial;

		  private:
			// UI rendering methods
			void RenderHierarchyWindow();
			void RenderTopBar();
			void RenderAnimationWindow();
			void RenderAudioDebugUI();
			void RenderPauseOverlay();
			void RenderSceneView(GLuint texId);


			std::unique_ptr<AssetUIRenderer>   m_uiAssetRenderer;
			std::unique_ptr<InspectorRenderer> m_inspectorRenderer;
			std::unique_ptr<MaterialEditor>    m_materialEditor;
			// Selected entity
			Entity m_selectedEntity;
		};
	} // namespace UI
} // namespace Engine