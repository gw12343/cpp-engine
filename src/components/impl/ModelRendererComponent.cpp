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
#include "utils/ModelLoader.h"
#include "ModelRendererComponent.h"

namespace Engine::Components {

	void ModelRenderer::AddBindings()
	{
		auto& lua = GetScriptManager().lua;

		lua.new_usertype<ModelRenderer>("ModelRenderer", "setModel", &ModelRenderer::SetModel);
	}

	void ModelRenderer::SetModel(std::string path)
	{
		model = Rendering::ModelLoader::LoadModel(path);
	}


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