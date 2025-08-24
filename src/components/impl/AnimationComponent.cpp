//
// Created by gabe on 6/24/25.
//

#include "components/Components.h"
#include "components/impl/AnimationComponent.h"

#include "core/Engine.h"
#include "core/Entity.h"
#include "utils/Utils.h"
#include "imgui.h"
#include "ozz/animation/runtime/track.h"
#include "rendering/particles/ParticleManager.h"
#include "animation/AnimationManager.h"
#include "scripting/ScriptManager.h"

#include "misc/cpp/imgui_stdlib.h"

namespace Engine::Components {

	void AnimationComponent::OnAdded(Entity& entity)
	{
		if (!animationPath.empty()) {
			animation = GetAnimationManager().LoadAnimationFromPath(animationPath);
			if (!animation) {
				spdlog::error("Failed to load animation from path: {}", animationPath);
			}
			else {
				GetDefaultLogger()->info("Loaded animation from path: {}", animationPath);
			}
		}
	}

	void AnimationComponent::OnRemoved(Entity& entity)
	{
	}

	void AnimationComponent::RenderInspector(Entity& entity)
	{
		ImGui::Text("Animation: %s", animation ? "Loaded" : "Null");
		ImGui::Text("Tracks: %d", animation ? animation->num_tracks() : 0);
		ImGui::Text("Duration: %.2f", animation ? animation->duration() : 0.0f);
		ImGui::Separator();
		if (ImGui::InputText("Animation Path", &animationPath)) {
			animation = GetAnimationManager().LoadAnimationFromPath(animationPath);
			if (!animation) {
				spdlog::error("Failed to load animation from path: {}", animationPath);
			}
			else {
				GetDefaultLogger()->info("Loaded animation from path: {}", animationPath);
			}
		}
	}

} // namespace Engine::Components