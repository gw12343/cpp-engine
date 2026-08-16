//
// Created by gabe on 6/24/25.
//
#include "core/Entity.h"



#include "ozz/animation/runtime/track.h"
#include "rendering/particles/ParticleManager.h"
#include "animation/AnimationManager.h"
#include "scripting/ScriptManager.h"

#include "ShadowCasterComponent.h"

namespace Engine::Components {
	void ShadowCaster::OnRemoved(Entity& entity)
	{
	}
	void ShadowCaster::OnAdded(Entity& entity)
	{
	}

	void ShadowCaster::RenderInspector(Entity& /*entity*/)
	{
		ImGui::TextWrapped("This entity contributes to the directional shadow cascade. "
		                   "Place it on lights or large occluders you want in the shadow map.");
		ImGui::TextDisabled("Cascade resolution and splits are in Editor Settings / RenderSettings.");
	}
} // namespace Engine::Components