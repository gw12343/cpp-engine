//
// Created by gabe on 6/24/25.
//

#include "SkinnedMeshComponent.h"

#include "core/Engine.h"
#include "core/EngineData.h"
#include "core/Entity.h"
#include "core/Scene.h"

#include "ozz/animation/runtime/track.h"
#include "ozz/base/containers/vector.h"
#include "ozz/base/maths/simd_math.h"

#include "animation/AnimationManager.h"
#include "rendering/ui/InspectorUI.h"

namespace Engine::Components {
	std::unordered_set<std::vector<ozz::math::Float4x4>*> SkinnedMeshComponent::s_skin_mats;
	std::unordered_set<ozz::vector<Engine::AnimatedMesh>*>        SkinnedMeshComponent::s_all_meshes;

	void SkinnedMeshComponent::FreeMeshes()
	{
		if (meshes) {
			s_all_meshes.erase(meshes);
			delete meshes;
			meshes = nullptr;
		}
		skin_frame_cache.clear();
		skin_cache_frame = 0;
	}

	void SkinnedMeshComponent::FreeSkinningMatrices()
	{
		if (skinning_matrices) {
			s_skin_mats.erase(skinning_matrices);
			delete skinning_matrices;
			skinning_matrices = nullptr;
		}
	}

	void SkinnedMeshComponent::RebuildSkinningMatrices()
	{
		FreeSkinningMatrices();

		if (!meshes || meshes->empty()) {
			return;
		}

		skinning_matrices = new std::vector<ozz::math::Float4x4>();
		s_skin_mats.insert(skinning_matrices);

		// A mesh is skinned by only a subset of joints; joint_remaps size is the
		// number of matrices required for that mesh.
		size_t num_skinning_matrices = 0;
		for (const Engine::AnimatedMesh& mesh : *meshes) {
			num_skinning_matrices = ozz::math::Max(num_skinning_matrices, mesh.joint_remaps.size());
		}

		skinning_matrices->resize(num_skinning_matrices);

		// One frame cache slot per mesh
		skin_frame_cache.clear();
		skin_frame_cache.resize(meshes->size());
		skin_cache_frame = 0;
	}

	bool SkinnedMeshComponent::TryLoadMeshes()
	{
		FreeMeshes();

		if (meshPath.empty()) {
			FreeSkinningMatrices();
			return false;
		}

		meshes = AnimationManager::LoadMeshesFromPath(meshPath);
		if (!meshes) {
			GetDefaultLogger()->error("SkinnedMeshComponent: Failed to load meshes from path: {}", meshPath);
			FreeSkinningMatrices();
			return false;
		}

		s_all_meshes.insert(meshes);
		RebuildSkinningMatrices();
		return true;
	}

	void SkinnedMeshComponent::SetMeshPath(const std::string& path)
	{
		meshPath = path;
		TryLoadMeshes();
	}

	void SkinnedMeshComponent::OnRemoved(Entity& entity)
	{
		GetDefaultLogger()->info("REMOVING SKINNED MESH COMPONENT");
		FreeSkinningMatrices();
		FreeMeshes();
	}

	void SkinnedMeshComponent::OnAdded(Entity& entity)
	{
		// Allow adding from the inspector with an empty path. Meshes can be loaded
		// later via SetMeshPath / the inspector "Load Mesh" button.
		if (!meshes) {
			if (meshPath.empty()) {
				return;
			}
			// Load may fail; keep the component alive so the path can be fixed in-editor.
			TryLoadMeshes();
			return;
		}

		// Meshes already assigned (e.g. constructor) — track them and ensure matrices.
		s_all_meshes.insert(meshes);
		if (!skinning_matrices) {
			RebuildSkinningMatrices();
		}
	}

	void SkinnedMeshComponent::RenderInspector(Entity& entity)
	{
		LeftLabelCheckbox("Visible", &visible);

		LeftLabelInputText("Mesh Path", &meshPath);
		ImGui::SameLine();
		BrowsePathButton("mesh", "ozz,fbx", "resources/animations", &meshPath);
		if (ImGui::Button("Load Mesh")) {
			TryLoadMeshes();
		}
		ImGui::SameLine();
		if (ImGui::Button("Clear Mesh")) {
			meshPath.clear();
			FreeMeshes();
			FreeSkinningMatrices();
		}

		ImGui::Text("Meshes: %s", meshes ? std::to_string(meshes->size()).c_str() : "None");
		ImGui::Text("Skinning Matrices: %s",
		            skinning_matrices ? std::to_string(skinning_matrices->size()).c_str() : "None");
		LeftLabelAssetMaterial("Material", &meshMaterial);
	}

	void SkinnedMeshComponent::CleanSkinnedModels()
	{
		if (Get().scene && Get().assetManager) {
			Scene* scene = GetCurrentScene();
			if (scene && scene->GetRegistry()) {
				auto view = scene->GetRegistry()->view<SkinnedMeshComponent>();
				for (auto entity : view) {
					auto& sm = view.get<SkinnedMeshComponent>(entity);
					sm.skinning_matrices = nullptr;
					sm.meshes            = nullptr;
				}
			}
		}

		for (std::vector<ozz::math::Float4x4>* mat : s_skin_mats) {
			delete mat;
		}
		s_skin_mats.clear();

		for (ozz::vector<Engine::AnimatedMesh>* mesh : s_all_meshes) {
			delete mesh;
		}
		s_all_meshes.clear();
	}


} // namespace Engine::Components
