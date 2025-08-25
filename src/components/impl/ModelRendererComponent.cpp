//
// Created by gabe on 6/24/25.
//
#include "core/Entity.h"
#include "utils/Utils.h"

#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"
#include "rendering/particles/ParticleManager.h"
#include "animation/AnimationManager.h"
#include "scripting/ScriptManager.h"
#include "assets/impl/ModelLoader.h"
#include "ModelRendererComponent.h"
#include "rendering/ui/InspectorUI.h"
#include "core/EngineData.h"
namespace Engine::Components {

	void ModelRenderer::AddBindings()
	{
		auto& lua = GetScriptManager().lua;

		lua.new_usertype<ModelRenderer>("ModelRenderer", "setModel", &ModelRenderer::SetModel);
	}

	void ModelRenderer::Draw(const Shader& shader, const Components::Transform& transform, bool uploadMaterial) const
	{
		if (visible && model.IsValid()) {
			const auto* actualModel = GetAssetManager().Get(model);
			if (!actualModel) return;

			// Set model matrix in shader
			shader.Bind();
			glm::mat4 modelMatrix = transform.GetMatrix();
			shader.SetMat4("model", &modelMatrix);

			// Draw the model
			actualModel->Draw(shader, backfaceCulling, uploadMaterial, materialOverrides);
		}
	}

	void ModelRenderer::SetModel(const std::string& path)
	{
		model = GetAssetManager().Load<Rendering::Model>(path);
		if (model.IsValid()) {
			Rendering::Model* m = GetAssetManager().Get(model);
			if (m != NULL) {
				materialOverrides.resize(m->GetMeshes().size());
			}
		}
	}


	void ModelRenderer::OnRemoved(Entity& entity)
	{
	}
	void ModelRenderer::OnAdded(Entity& entity)
	{
		if (model.IsValid()) {
			Rendering::Model* m = GetAssetManager().Get(model);
			if (m != nullptr) {
				materialOverrides.resize(m->GetMeshes().size());
			}
		}
	}


	void ModelRenderer::RenderInspector(Entity& entity)
	{
		LeftLabelCheckbox("Visible", &visible);
		LeftLabelCheckbox("Cull Backface", &backfaceCulling);

		if (LeftLabelModelAsset("Model", &model)) {
			Rendering::Model* m = GetAssetManager().Get(model);
			if (m != nullptr) {
				materialOverrides.resize(m->GetMeshes().size());
			}
		}

		std::string newID;

		if (model.IsValid()) {
			Rendering::Model* m = GetAssetManager().Get(model);
			if (m != nullptr) {
				for (int i = 0; i < m->GetMeshes().size(); i++) {
					LeftLabelMaterialAsset(("Material " + std::to_string(i)).c_str(), &materialOverrides[i]);

					//					newID = materialOverrides[i].GetID();
					//					if (ImGui::InputText(("Material " + std::to_string(i)).c_str(), &newID)) {
					//						materialOverrides[i] = AssetHandle<Material>(newID);
					//					}
					//
					//					// Handle drag-and-drop on same item
					//					if (ImGui::BeginDragDropTarget()) {
					//						struct PayloadData {
					//							const char* type;
					//							char        id[64];
					//						};
					//
					//						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_MATERIAL")) {
					//							if (payload->DataSize == sizeof(PayloadData)) {
					//								const auto* data = static_cast<const PayloadData*>(payload->Data);
					//								// Extra safety: check type string
					//								if (std::strcmp(data->type, "Material") == 0) {
					//									materialOverrides[i] = AssetHandle<Material>(data->id);
					//									newID                = data->id;
					//								}
					//							}
					//						}
					//						ImGui::EndDragDropTarget();
					//					}
				}
			}
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
