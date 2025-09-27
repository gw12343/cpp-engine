//
// Created by gabe on 8/27/25.
//

#include "NavigationComponents.h"
#include <imgui.h>
#include "rendering/ui/InspectorUI.h"
#include "navigation/NavigationManager.h"

namespace Engine {
	namespace Components {
		void NavMeshGeometry::RenderInspector(Entity& entity)
		{
			ImGui::Text("NavMesh Geometry");
			LeftLabelCheckbox("Is Walkable", &isWalkable);
			ImGui::InputInt("Area", &area);
			ImGui::TextWrapped("This component marks geometry for inclusion in navigation mesh generation.");
		}

		void NavAgent::RenderInspector(Entity& entity)
		{
			ImGui::Text("Navigation Agent");
			LeftLabelSliderFloat("Radius", &radius, 0.1f, 2.0f);
			LeftLabelSliderFloat("Height", &height, 0.5f, 5.0f);
			LeftLabelSliderFloat("Max Speed", &maxSpeed, 0.1f, 10.0f);

			ImGui::Text("Target: (%.1f, %.1f, %.1f)", targetPosition.x, targetPosition.y, targetPosition.z);
			ImGui::Text("Has Path: %s", hasPath ? "Yes" : "No");
			ImGui::Text("Is Moving: %s", isMoving ? "Yes" : "No");

			if (ImGui::Button("Stop")) {
				isMoving = false;
				hasPath  = false;
				path.clear();
			}

			if (ImGui::Button("Send Random")) {
				GetNav().SetAgentTarget(entity, GetNav().GetRandomPoint());
			}

			if (hasPath) {
				ImGui::Text("Path Nodes: %d", path.size());
				for (auto p : path) {
					ImGui::Text("(%f, %f, %f)", p.x, p.y, p.z);
				}
			}
		}
	} // namespace Components
} // namespace Engine