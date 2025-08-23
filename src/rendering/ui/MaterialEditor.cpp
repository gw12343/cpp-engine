//
// Created by gabe on 8/23/25.
//

#include "MaterialEditor.h"
#include "imgui.h"
#include "IconsFontAwesome6.h"
#include "rendering/Texture.h"
#include "rendering/Material.h"
#include "core/EngineData.h"
#include "assets/AssetManager.h"
#include "misc/cpp/imgui_stdlib.h"

namespace Engine {

	void MaterialEditor::RenderMaterialEditor(AssetHandle<Material> matRef)
	{
		ImGui::Begin(ICON_FA_PALETTE " Material Editor");

		if (!matRef.IsValid()) {
			ImGui::Text("Invalid material selected.");
			ImGui::End();
			return;
		}

		Material* material = GetAssetManager().Get(matRef);


		// --- Material name ---
		{
			std::string name = material->GetName();
			if (ImGui::InputText("Name", &name)) {
				material->SetName(name);
			}
		}

		ImGui::Separator();

		// --- Textures ---
		auto textureField = [&](const char* label, AssetHandle<Texture> tex, auto setter) {
			std::string texID = tex.GetID();
			if (ImGui::InputText(label, &texID)) {
				setter(AssetHandle<Texture>(texID));
			}

			if (ImGui::BeginDragDropTarget()) {
				struct PayloadData {
					const char* type;
					char        id[64];
				};

				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_TEXTURE")) {
					if (payload->DataSize == sizeof(PayloadData)) {
						const PayloadData* data = static_cast<const PayloadData*>(payload->Data);
						if (std::strcmp(data->type, "Texture") == 0) {
							setter(AssetHandle<Texture>(data->id));
							texID = data->id;
						}
					}
				}
				ImGui::EndDragDropTarget();
			}
		};

		textureField("Diffuse Texture", material->GetDiffuseTexture(), [&](AssetHandle<Texture> t) { material->SetDiffuseTexture(t); });

		textureField("Specular Texture", material->GetSpecularTexture(), [&](AssetHandle<Texture> t) { material->SetSpecularTexture(t); });

		textureField("Normal Texture", material->GetNormalTexture(), [&](AssetHandle<Texture> t) { material->SetNormalTexture(t); });

		textureField("Height Texture", material->GetHeightTexture(), [&](AssetHandle<Texture> t) { material->SetHeightTexture(t); });

		ImGui::Separator();

		// --- Colors ---
		glm::vec3 diffuse  = material->GetDiffuseColor();
		glm::vec3 specular = material->GetSpecularColor();
		glm::vec3 ambient  = material->GetAmbientColor();
		glm::vec3 emissive = material->GetEmissiveColor();

		if (ImGui::ColorEdit3("Diffuse Color", &diffuse.x)) material->SetDiffuseColor(diffuse);
		if (ImGui::ColorEdit3("Specular Color", &specular.x)) material->SetSpecularColor(specular);
		if (ImGui::ColorEdit3("Ambient Color", &ambient.x)) material->SetAmbientColor(ambient);
		if (ImGui::ColorEdit3("Emissive Color", &emissive.x)) material->SetEmissiveColor(emissive);

		float shininess = material->GetShininess();
		if (ImGui::SliderFloat("Shininess", &shininess, 0.0f, 512.0f)) {
			material->SetShininess(shininess);
		}

		ImGui::End();
	}
} // namespace Engine

#include "assets/AssetManager.inl"