#pragma once

#include "Camera.h"
#include "animation/renderer_impl.h"
#include "core/Entity.h"
#include "sound/SoundManager.h"

#include <entt/entt.hpp>
#include <imgui.h>
#include <memory>

namespace Engine {
	class GEngine;
	namespace UI {

		class UIManager {
		  public:
			UIManager(GEngine* engine);
			~UIManager() = default;

			void Initialize();
			void Render();

		  private:
			// UI rendering methods
			void RenderHierarchyWindow();
			void RenderInspectorWindow();
			void RenderAnimationWindow();
			void RenderAudioDebugUI();
			void RenderPauseOverlay();

			// Reference to the engine
			GEngine* m_engine;

			// Selected entity
			Entity m_selectedEntity;
		};
	} // namespace UI
} // namespace Engine