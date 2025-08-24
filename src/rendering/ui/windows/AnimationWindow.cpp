//
// Created by gabe on 8/24/25.
//

#include "AnimationWindow.h"
#include "core/EngineData.h"
#include "imgui.h"
#include "animation/AnimationManager.h"

namespace Engine {
	void DrawAnimationWindow()
	{
		ImGui::Begin("Animation");

		if (ImGui::CollapsingHeader("Animation")) {
			//						auto& animManager = GetAnimationManager();
			//
			//
			//						auto& skeleton = animManager.GetSkeleton();
			//						auto& meshes   = animManager.GetMeshes();
			//
			//						ImGui::LabelText("Animated Joints", "%d animated joints", skeleton.num_joints());
			//
			//						int influences = 0;
			//						for (const auto& mesh : meshes) {
			//							influences = ozz::math::Max(influences, mesh.max_influences_count());
			//						}
			//						ImGui::LabelText("Influences", "%d influences (max)", influences);
			//
			//						int vertices = 0;
			//						for (const auto& mesh : meshes) {
			//							vertices += mesh.vertex_count();
			//						}
			//
			//						ImGui::LabelText("Vertices", "%.1fK vertices", vertices / 1000.f);
			//
			//						int indices = 0;
			//						for (const auto& mesh : meshes) {
			//							indices += mesh.triangle_index_count();
			//						}
			//
			//						ImGui::LabelText("Triangles", "%.1fK triangles", indices / 3000.f);
		}

		if (ImGui::CollapsingHeader("Rendering Options")) {
			auto& animManager    = GetAnimationManager();
			auto& draw_skeleton  = animManager.GetDrawSkeleton();
			auto& draw_mesh      = animManager.GetDrawMesh();
			auto& render_options = animManager.GetRenderOptions();

			ImGui::Checkbox("Show Skeleton", &draw_skeleton);
			ImGui::Checkbox("Show Mesh", &draw_mesh);
			ImGui::Separator();

			ImGui::Checkbox("Show triangles", &render_options.triangles);
			ImGui::Checkbox("Show texture", &render_options.texture);
			ImGui::Checkbox("Show vertices", &render_options.vertices);
			ImGui::Checkbox("Show normals", &render_options.normals);
			ImGui::Checkbox("Show tangents", &render_options.tangents);
			ImGui::Checkbox("Show binormals", &render_options.binormals);
			ImGui::Checkbox("Show colors", &render_options.colors);
			ImGui::Checkbox("Wireframe", &render_options.wireframe);
			ImGui::Checkbox("Skip skinning", &render_options.skip_skinning);
		}

		ImGui::SliderFloat("fov", &GetCamera().m_fov, 45, 120);

		ImGui::End();
	}
} // namespace Engine