#pragma once

#include "Camera.h"
#include "animation/renderer_impl.h"
#include "core/Entity.h"
#include "sound/SoundManager.h"

#include "entt/entt.hpp"
#include "imgui.h"
#include "core/module/Module.h"
#include <memory>

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


			// Selected entity
			Entity m_selectedEntity;
		};
	} // namespace UI
} // namespace Engine