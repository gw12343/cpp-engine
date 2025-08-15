#pragma once

#include "Camera.h"
#include "animation/renderer_impl.h"
#include "core/Entity.h"
#include "sound/SoundManager.h"

#include "entt/entt.hpp"
#include "imgui.h"
#include "core/module/Module.h"
#include "ModelPreview.h"
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


			void DrawTopBar();
			void BeginDockspace();
			void EndDockspace();

		  private:
			// UI rendering methods
			void RenderHierarchyWindow();
			void RenderInspectorWindow();
			void RenderAnimationWindow();
			void RenderAudioDebugUI();
			void RenderPauseOverlay();
			void RenderSceneView(GLuint texId);
			void RenderAssetWindow();

			void DrawSoundAssets();
			void DrawTextureAssets();
			void DrawModelAssets();

			// Selected entity
			Entity m_selectedEntity;

			std::unordered_map<std::type_index, std::function<void()>> drawFuncs;
			std::unordered_map<std::string, ModelPreview>              m_modelPreviews;
		};
	} // namespace UI
} // namespace Engine