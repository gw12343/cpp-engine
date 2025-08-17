//
// Created by gabe on 6/24/25.
//
#include "core/Entity.h"
#include "utils/Utils.h"

#include "imgui.h"
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

	void ShadowCaster::RenderInspector(Entity& entity)
	{
		ImGui::Text("hi");
	}
} // namespace Engine::Components