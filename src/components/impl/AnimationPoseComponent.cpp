//
// Created by gabe on 6/24/25.
//

#include "AnimationPoseComponent.h"

#include "core/Engine.h"
#include "core/Entity.h"
#include "utils/Utils.h"

#include "imgui.h"
#include "ozz/animation/runtime/track.h"
#include <string>
#include "rendering/particles/ParticleManager.h"
#include "animation/AnimationManager.h"
#include "scripting/ScriptManager.h"
#include "SkeletonComponent.h"

namespace Engine::Components {

	void AnimationPoseComponent::OnRemoved(Entity& entity)
	{
		GetDefaultLogger()->info("DELETING ANIMATION POSES");
		if (local_pose) {
			delete local_pose;
			local_pose = nullptr;
		}

		if (model_pose) {
			delete model_pose;
			model_pose = nullptr;
		}
	}

	void AnimationPoseComponent::OnAdded(Entity& entity)
	{
		ENGINE_VERIFY(entity.HasComponent<SkeletonComponent>(), "AnimationPoseComponent::OnAdded: Missing SkeletonComponent");
		auto& skeletonComponent = entity.GetComponent<SkeletonComponent>();
		ENGINE_VERIFY(skeletonComponent.skeleton != nullptr, "AnimationPoseComponent::OnAdded: SkeletonComponent has null skeleton");


		// Allocate pose data
		local_pose = AnimationManager::AllocateLocalPose(skeletonComponent.skeleton);
		model_pose = AnimationManager::AllocateModelPose(skeletonComponent.skeleton);

		if (!local_pose || !model_pose) {
			spdlog::error("Failed to allocate pose data for entity");
		}
	}

	void AnimationPoseComponent::RenderInspector(Entity& entity)
	{
		ImGui::Text("Local Pose: %s", local_pose ? std::to_string(local_pose->size()).c_str() : "Null");
		ImGui::Text("Model Pose: %s", model_pose ? std::to_string(model_pose->size()).c_str() : "Null");
	}

} // namespace Engine::Components