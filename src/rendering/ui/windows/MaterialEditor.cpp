//
// Created by gabe on 8/23/25.
//

#include "MaterialEditor.h"

#include "rendering/ui/IconsFontAwesome6.h"

#include "rendering/Texture.h"
#include "rendering/Material.h"
#include "core/EngineData.h"

#include "misc/cpp/imgui_stdlib.h"
#include "assets/impl/MaterialLoader.h"
#include "rendering/Renderer.h"
#include "rendering/ui/UIManager.h"
#include <algorithm>

namespace Engine {

	void MaterialEditor::RenderMaterialEditor(MaterialHandle matRef)
	{
		const char* title = m_dirty ? ICON_FA_PALETTE " Material Editor*" : ICON_FA_PALETTE " Material Editor";
		ImGui::Begin(title);

		auto loader = ((MaterialLoader*) (GetAssetManager().GetStorage<Material>().loader.get()));

		if (!matRef.IsValid()) {
			ImGui::Text("No material selected. Click a .material in Assets.");
		}
		else {
			Material* material = GetAssetManager().Get(matRef);

			// Two-column layout: properties on left, preview on right
			float availWidth = ImGui::GetContentRegionAvail().x;
			float previewSize = std::clamp(availWidth * 0.4f, 180.0f, 420.0f);
			float propertiesWidth = availWidth - previewSize - 20.0f; // 20px spacing

			// Left column: Material properties
			ImGui::BeginChild("PropertiesPanel", ImVec2(propertiesWidth, 0), false);

			// --- Material name ---
			{
				std::string name = material->GetName();
				if (LeftLabelInputText("Name", &name)) {
					material->SetName(name);
					m_dirty = true;
				}
			}

			ImGui::Separator();

			m_dirty |= LeftLabelAssetTexture("Diffuse Texture", &material->m_diffuseTexture);
			m_dirty |= LeftLabelAssetTexture("Normal Texture", &material->m_normalTexture);
			m_dirty |= LeftLabelAssetTexture("Specular Texture", &material->m_specularTexture);
			m_dirty |= LeftLabelAssetTexture("Height Texture", &material->m_heightTexture);

			ImGui::Separator();

			// --- Colors ---
			glm::vec3 diffuse  = material->GetDiffuseColor();
			glm::vec3 specular = material->GetSpecularColor();
			glm::vec3 ambient  = material->GetAmbientColor();
			glm::vec3 emissive = material->GetEmissiveColor();

			if (LeftLabelColorEdit3("Diffuse Color", &diffuse.x)) { material->SetDiffuseColor(diffuse); m_dirty = true; }
			if (LeftLabelColorEdit3("Specular Color", &specular.x)) { material->SetSpecularColor(specular); m_dirty = true; }
			if (LeftLabelColorEdit3("Ambient Color", &ambient.x)) { material->SetAmbientColor(ambient); m_dirty = true; }
			if (LeftLabelColorEdit3("Emissive Color", &emissive.x)) { material->SetEmissiveColor(emissive); m_dirty = true; }

			glm::vec2 scale = material->GetTextureScale();
			if (LeftLabelDragFloat2("Texture Scale", &scale.x, 0.05, 0.0)) { material->SetTextureScale(scale); m_dirty = true; }


			float shininess = material->GetShininess();
			if (LeftLabelSliderFloat("Shininess", &shininess, 0.0f, 512.0f)) {
				material->SetShininess(shininess);
				m_dirty = true;
			}

			ImGui::Spacing();
			if (ImGui::Button(m_dirty ? "Save*" : "Save")) {
				loader->SaveMaterial(*material, material->m_path);
				m_dirty = false;
			}

			ImGui::EndChild();

			// Right column: Preview
			ImGui::SameLine();
			RenderPreviewPanel(material);
		}

		if (ImGui::Button("Create New Material")) {
			m_promptNew = true;
			std::snprintf(m_newName, sizeof(m_newName), "NewMaterial");
		}
		if (m_promptNew) {
			ImGui::OpenPopup("New Material");
			m_promptNew = false;
		}
		if (ImGui::BeginPopupModal("New Material", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::InputText("Name", m_newName, sizeof(m_newName));
			if (ImGui::Button("Create") || ImGui::IsKeyPressed(ImGuiKey_Enter)) {
				std::string result = m_newName;
				if (result.empty()) result = "NewMaterial";
				Material newMat;
				newMat.m_path = "resources/materials/" + result + ".material";
				newMat.SetName(result);
				loader->SaveMaterial(newMat, newMat.m_path);
				GetUI().m_selectedMaterial = GetAssetManager().Load<Material>(newMat.m_path);
				m_dirty = false;
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
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