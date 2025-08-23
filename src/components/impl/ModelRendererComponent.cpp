//
// Created by gabe on 6/24/25.
//
#include "core/Entity.h"
#include "utils/Utils.h"

#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"
#include "ozz/animation/runtime/track.h"
#include "rendering/particles/ParticleManager.h"
#include "animation/AnimationManager.h"
#include "scripting/ScriptManager.h"
#include "assets/impl/ModelLoader.h"
#include "ModelRendererComponent.h"

namespace Engine::Components {

	void ModelRenderer::AddBindings()
	{
		auto& lua = GetScriptManager().lua;

		lua.new_usertype<ModelRenderer>("ModelRenderer", "setModel", &ModelRenderer::SetModel);
	}

	void ModelRenderer::Draw(const Shader& shader, const Components::Transform& transform) const
	{
		if (visible && model.IsValid()) {
			const auto* actualModel = GetAssetManager().Get(model);
			if (!actualModel) return;

			// Set model matrix in shader
			shader.Bind();
			glm::mat4 modelMatrix = transform.GetMatrix();
			shader.SetMat4("model", &modelMatrix);

			// Draw the model
			actualModel->Draw(shader, materialOverride);
		}
	}

	void ModelRenderer::SetModel(const std::string& path)
	{
		model = GetAssetManager().Load<Rendering::Model>(path);
	}


	void ModelRenderer::OnRemoved(Entity& entity)
	{
	}
	void ModelRenderer::OnAdded(Entity& entity)
	{
	}


	void ModelRenderer::RenderInspector(Entity& entity)
	{
		ImGui::Checkbox("Visible", &visible);
		ImGui::Checkbox("Cull Backface", &backfaceCulling);

		std::string newID = model.GetID();
		if (ImGui::InputText("Model", &newID)) {
			model = AssetHandle<Rendering::Model>(newID);
		}

		// Handle drag-and-drop on same item
		if (ImGui::BeginDragDropTarget()) {
			struct PayloadData {
				const char* type;
				char        id[64];
			};

			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_MODEL")) {
				if (payload->DataSize == sizeof(PayloadData)) {
					const PayloadData* data = static_cast<const PayloadData*>(payload->Data);
					// Extra safety: check type string
					if (std::strcmp(data->type, "Rendering::Model") == 0) {
						model = AssetHandle<Rendering::Model>(data->id);
						newID = data->id;
					}
				}
			}
			ImGui::EndDragDropTarget();
		}

		newID = materialOverride.GetID();
		if (ImGui::InputText("Material", &newID)) {
			materialOverride = AssetHandle<Material>(newID);
		}

		// Handle drag-and-drop on same item
		if (ImGui::BeginDragDropTarget()) {
			struct PayloadData {
				const char* type;
				char        id[64];
			};

			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_MATERIAL")) {
				if (payload->DataSize == sizeof(PayloadData)) {
					const auto* data = static_cast<const PayloadData*>(payload->Data);
					// Extra safety: check type string
					if (std::strcmp(data->type, "Material") == 0) {
						materialOverride = AssetHandle<Material>(data->id);
						newID            = data->id;
					}
				}
			}
			ImGui::EndDragDropTarget();
		}

		if (model.IsValid()) {
			ImGui::Text("Model: Loaded");
			// TODO add more model info here
		}
		else {
			ImGui::Text("Model: Invalid");
		}
	}

} // namespace Engine::Components
#include "assets/AssetManager.inl"
