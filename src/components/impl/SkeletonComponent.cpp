//
// Created by gabe on 6/24/25.
//

#include "components/Components.h"
#include "SkeletonComponent.h"
#include "core/Engine.h"
#include "core/Entity.h"
#include "utils/Utils.h"

#include "imgui.h"
#include "ozz/animation/runtime/track.h"
#include "rendering/particles/ParticleManager.h"
#include "animation/AnimationManager.h"
#include "scripting/ScriptManager.h"

namespace Engine::Components {

	void SkeletonComponent::OnRemoved(Entity& entity)
	{
	}
	void SkeletonComponent::OnAdded(Entity& entity)
	{
		if (!skeletonPath.empty()) {
			skeleton = GetAnimationManager().LoadSkeletonFromPath(skeletonPath);
			if (!skeleton) {
				spdlog::error("Failed to load skeleton from path: {}", skeletonPath);
			}
			else {
				GetDefaultLogger()->info("Loaded skeleton from path: {}", skeletonPath);
			}
		}
	}


	void SkeletonComponent::RenderInspector(Entity& entity)
	{
		ImGui::Text("Skeleton: %s", skeleton ? "Loaded" : "Null");
		ImGui::Text("Joints: %d", skeleton ? skeleton->num_joints() : 0);
		ImGui::Text("SOA Joints: %d", skeleton ? skeleton->num_soa_joints() : 0);
	}


} // namespace Engine::Components