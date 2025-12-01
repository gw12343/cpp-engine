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

		lua.new_usertype<ModelRenderer>("ModelRenderer", "setModel", &ModelRenderer::SetModel, "setMaterial", &ModelRenderer::SetMaterial);
	}

	void ModelRenderer::Draw(const Shader& shader, Components::Transform& transform, bool uploadMaterial)
	{
		if (visible && model.IsValid()) {
			const auto* actualModel = GetAssetManager().Get(model);
			if (!actualModel) return;

			// Set model matrix in shader
			shader.Bind();
			glm::mat4 modelMatrix = transform.GetWorldMatrix();
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

		if (LeftLabelAssetModel("Model", &model)) {
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
					LeftLabelAssetMaterial(("Material " + std::to_string(i)).c_str(), &materialOverrides[i]);
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
	void ModelRenderer::SetMaterial(AssetHandle<Material> mat)
	{
		// resize material overrides to match model
		if (model.IsValid()) {
			Rendering::Model* m = GetAssetManager().Get(model);
			if (m != nullptr) {
				materialOverrides.resize(m->GetMeshes().size());
			}
		}


		// replace all materials with this one
		for (auto& m : materialOverrides) {
			m = mat;
		}
	}

} // namespace Engine::Components
#include "assets/AssetManager.inl"
