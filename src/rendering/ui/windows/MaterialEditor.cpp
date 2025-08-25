//
// Created by gabe on 8/23/25.
//

#include "MaterialEditor.h"
#include "imgui.h"
#include "rendering/ui/IconsFontAwesome6.h"
#include "rendering/ui/InspectorUI.h"
#include "rendering/Texture.h"
#include "rendering/Material.h"
#include "core/EngineData.h"
#include "assets/AssetManager.h"
#include "misc/cpp/imgui_stdlib.h"
#include "assets/impl/MaterialLoader.h"

namespace Engine {

	void MaterialEditor::RenderMaterialEditor(AssetHandle<Material> matRef)
	{
		ImGui::Begin(ICON_FA_PALETTE " Material Editor");

		auto loader = ((MaterialLoader*) (GetAssetManager().GetStorage<Material>().loader.get()));

		if (!matRef.IsValid()) {
			ImGui::Text("Invalid material selected.");
		}
		else {
			Material* material = GetAssetManager().Get(matRef);


			// --- Material name ---
			{
				std::string name = material->GetName();
				if (LeftLabelInputText("Name", &name)) {
					material->SetName(name);
				}
			}

			ImGui::Separator();

			LeftLabelAssetTexture("Diffuse Texture", &material->m_diffuseTexture);
			LeftLabelAssetTexture("Normal Texture", &material->m_normalTexture);
			LeftLabelAssetTexture("Specular Texture", &material->m_specularTexture);
			LeftLabelAssetTexture("Height Texture", &material->m_heightTexture);

			ImGui::Separator();

			// --- Colors ---
			glm::vec3 diffuse  = material->GetDiffuseColor();
			glm::vec3 specular = material->GetSpecularColor();
			glm::vec3 ambient  = material->GetAmbientColor();
			glm::vec3 emissive = material->GetEmissiveColor();

			if (LeftLabelColorEdit3("Diffuse Color", &diffuse.x)) material->SetDiffuseColor(diffuse);
			if (LeftLabelColorEdit3("Specular Color", &specular.x)) material->SetSpecularColor(specular);
			if (LeftLabelColorEdit3("Ambient Color", &ambient.x)) material->SetAmbientColor(ambient);
			if (LeftLabelColorEdit3("Emissive Color", &emissive.x)) material->SetEmissiveColor(emissive);

			glm::vec2 scale = material->GetTextureScale();
			if (LeftLabelDragFloat2("Texture Scale", &scale.x, 0.05, 0.0)) material->SetTextureScale(scale);


			float shininess = material->GetShininess();
			if (LeftLabelSliderFloat("Shininess", &shininess, 0.0f, 512.0f)) {
				material->SetShininess(shininess);
			}

			if (ImGui::Button("Save")) {
				loader->SaveMaterial(*material, material->m_path);
			}
		}

		if (ImGui::Button("Create New Material+")) {
			static const char chars[] = "0123456789"
			                            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
			                            "abcdefghijklmnopqrstuvwxyz";

			static thread_local std::mt19937       rng{std::random_device{}()};
			static std::uniform_int_distribution<> dist(0, sizeof(chars) - 2);

			std::string result = "mat";
			for (int i = 0; i < 5; i++)
				result += chars[dist(rng)];

			Material newMat;

			newMat.m_path = "resources/materials/" + result + ".material";
			newMat.SetName(result);


			loader->SaveMaterial(newMat, newMat.m_path);
			GetAssetManager().Load<Material>(newMat.m_path);
		}

		ImGui::End();
	}
} // namespace Engine

#include "assets/AssetManager.inl"