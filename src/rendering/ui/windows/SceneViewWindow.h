//
// Created by gabe on 8/24/25.
//

#ifndef CPP_ENGINE_SCENEVIEWWINDOW_H
#define CPP_ENGINE_SCENEVIEWWINDOW_H


#include "imgui.h"
#include "imgui_internal.h"
#include "imguizmo/ImGuizmo.h"

namespace Engine {
	void                       DrawSceneViewWindow();
	static ImGuizmo::OPERATION mCurrentGizmoOperation(ImGuizmo::TRANSLATE);
	static ImGuizmo::MODE      mCurrentGizmoMode(ImGuizmo::LOCAL);
} // namespace Engine

#endif // CPP_ENGINE_SCENEVIEWWINDOW_H
