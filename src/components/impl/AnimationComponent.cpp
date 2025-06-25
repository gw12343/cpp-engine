//
// Created by gabe on 6/24/25.
//

#include "components/Components.h"

#include "core/Engine.h"
#include "core/Entity.h"
#include "utils/Utils.h"
#include "imgui.h"
#include "ozz/animation/runtime/track.h"
#include "rendering/particles/ParticleManager.h"
#include "animation/AnimationManager.h"
#include "scripting/ScriptManager.h"

namespace Engine::Components {

	void AnimationComponent::OnAdded(Entity& entity)
	{
		if (!animationPath.empty()) {
			animation = GetAnimationManager().LoadAnimationFromPath(animationPath);
			if (!animation) {
				spdlog::error("Failed to load animation from path: {}", animationPath);
			}
			else {
				SPDLOG_INFO("Loaded animation from path: {}", animationPath);
			}
		}
	}

	void AnimationComponent::RenderInspector(Entity& entity)
	{
		ImGui::Text("Animation: %s", animation ? "Loaded" : "Null");
		ImGui::Text("Tracks: %d", animation ? animation->num_tracks() : 0);
		ImGui::Text("Duration: %.2f", animation ? animation->duration() : 0.0f);
	}

} // namespace Engine::Components