#pragma once

#include <memory>
#include <imgui.h>
#include <entt/entt.hpp>
#include "Camera.h"
#include "core/Entity.h"
#include "sound/SoundManager.h"
#include "animation/renderer_impl.h"

namespace Engine {

class GEngine;

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

}