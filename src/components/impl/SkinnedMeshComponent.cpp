//
// Created by gabe on 6/24/25.
//

#include "SkinnedMeshComponent.h"

#include "core/Engine.h"
#include "core/Entity.h"
#include "utils/Utils.h"
#include "imgui.h"
#include "ozz/animation/runtime/track.h"
#include "ozz/base/containers/vector.h"
#include "ozz/base/maths/simd_math.h"
#include <string>
#include "rendering/particles/ParticleManager.h"
#include "animation/AnimationManager.h"
#include "scripting/ScriptManager.h"

namespace Engine::Components {
	std::unordered_set<std::vector<ozz::math::Float4x4>*> SkinnedMeshComponent::s_skin_mats;
	std::unordered_set<ozz::vector<Engine::Mesh>*>        SkinnedMeshComponent::s_all_meshes;


	void SkinnedMeshComponent::OnRemoved(Entity& entity)
	{
	}
	void SkinnedMeshComponent::OnAdded(Entity& entity)
	{
		if (!meshPath.empty()) {
			meshes = AnimationManager::LoadMeshesFromPath(meshPath);
			s_all_meshes.insert(meshes);
			if (!meshes) {
				SPDLOG_ERROR("Failed to load meshes from path: {}", meshPath);
			}
			else {
				SPDLOG_INFO("Loaded SKINNED MESHES from path: {}", meshPath);
			}
		}

		ENGINE_VERIFY(meshes, "SkinnedMeshComponent::OnAdded: Failed to load meshes");

		skinning_matrices = new std::vector<ozz::math::Float4x4>();
		s_skin_mats.insert(skinning_matrices);
		// Computes the number of skinning matrices required to skin all meshes
		// A mesh is skinned by only a subset of joints, so the number of skinning
		// matrices might be less that the number of skeleton joints
		// Mesh::joint_remaps is used to know how to order skinning matrices. So
		// the number of matrices required is the size of joint_remaps
		size_t num_skinning_matrices = 0;
		for (const Engine::Mesh& mesh : *meshes) {
			num_skinning_matrices = ozz::math::Max(num_skinning_matrices, mesh.joint_remaps.size());
		}

		// Allocates skinning matrices
		skinning_matrices->resize(num_skinning_matrices);
		ENGINE_ASSERT(skinning_matrices, "SkinnedMeshComponent::OnAdded: Failed to allocate skinning matrices");
	}

	void SkinnedMeshComponent::RenderInspector(Entity& entity)
	{
		ImGui::Text("Meshes: %s", meshes ? std::to_string(meshes->size()).c_str() : "Null");
		ImGui::Text("Skinning Matrices: %s", skinning_matrices ? std::to_string(skinning_matrices->size()).c_str() : "Null");
	}


	void SkinnedMeshComponent::CleanSkinnedModels()
	{
		for (std::vector<ozz::math::Float4x4>* mat : s_skin_mats) {
			delete mat;
		}

		for (ozz::vector<Engine::Mesh>* mesh : s_all_meshes) {
			delete mesh;
		}
	}


} // namespace Engine::Components