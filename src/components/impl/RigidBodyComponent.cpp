//
// Created by gabe on 6/24/25.
//

#include "components/Components.h"

#include "core/Entity.h"
#include "utils/Utils.h"

#include "imgui.h"
#include "ozz/animation/runtime/track.h"
#include "rendering/particles/ParticleManager.h"
#include "animation/AnimationManager.h"
#include "scripting/ScriptManager.h"

namespace Engine::Components {

	void RigidBodyComponent::OnAdded(Entity& entity)
	{
	}


	void RigidBodyComponent::RenderInspector(Entity& entity)
	{
		ImGui::Text("Body ID: %u", bodyID.GetIndex());
		// Could add more physics properties here
	}

} // namespace Engine::Components