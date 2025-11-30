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
#include "rendering/Renderer.h"

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

			// Two-column layout: properties on left, preview on right
			float previewSize = 512.0f;
			float availWidth = ImGui::GetContentRegionAvail().x;
			float propertiesWidth = availWidth - previewSize - 20.0f; // 20px spacing

			// Left column: Material properties
			ImGui::BeginChild("PropertiesPanel", ImVec2(propertiesWidth, 0), false);

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

			ImGui::Spacing();
			if (ImGui::Button("Save")) {
				loader->SaveMaterial(*material, material->m_path);
			}

			ImGui::EndChild();

			// Right column: Preview
			ImGui::SameLine();
			RenderPreviewPanel(material);
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

	void MaterialEditor::RenderPreviewPanel(Material* material)
	{
		float previewSize = 512.0f;
		
		ImGui::BeginChild("PreviewPanel", ImVec2(previewSize, previewSize), true);
		
		ImGui::Text("Preview (Drag to Rotate)");
		ImGui::Separator();

		// Get the drawable region for the preview
		ImVec2 cursorPos = ImGui::GetCursorScreenPos();
		ImVec2 regionSize = ImGui::GetContentRegionAvail();
		float size = std::min(regionSize.x, regionSize.y - 30.0f); // Leave space for text

		// Create invisible button for drag interaction
		ImGui::InvisibleButton("PreviewDrag", ImVec2(size, size));

		// Handle mouse drag for rotation
		if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
			ImVec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
			m_previewYaw -= delta.x * 0.5f;  // Inverted for more natural feel
			m_previewPitch = glm::clamp(m_previewPitch - delta.y * 0.5f, -89.0f, 89.0f);
			ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
		}

		// Render the preview with current rotation
		if (!m_preview.initialized) {
			m_preview.width = static_cast<int>(previewSize);
			m_preview.height = static_cast<int>(previewSize);
			m_preview.Initialize();
		}

		m_preview.Render(material, GetRenderer().GetMaterialPreviewShader(), m_previewYaw, m_previewPitch);

		// Draw the preview texture
		ImGui::SetCursorScreenPos(cursorPos);
		void* texID = reinterpret_cast<void*>(static_cast<intptr_t>(m_preview.texture));
		ImGui::Image(texID, ImVec2(size, size), ImVec2(0, 1), ImVec2(1, 0)); // Y-flipped for framebuffer

		ImGui::EndChild();
	}
} // namespace Engine

#include "assets/AssetManager.inl"