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
	void ModelRenderer::OnAdded(Entity& entity)
	{
	}

	void ModelRenderer::RenderInspector(Entity& entity)
	{
		ImGui::Checkbox("Visible", &visible);

		if (model) {
			ImGui::Text("Model: Loaded");
			// Could add more model info here
		}
		else {
			ImGui::Text("Model: None");
		}
	}
} // namespace Engine::Components