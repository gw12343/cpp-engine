//
// Created by gabe on 8/21/25.
//

#include "InspectorRenderer.h"
#include "imgui.h"
#include "components/AllComponents.h"
#include "rendering/ui/IconsFontAwesome6.h"
#include "imgui_internal.h"
#include <string>
#include "misc/cpp/imgui_stdlib.h"
#include "rendering/ui/InspectorUI.h"


namespace Engine {

	static char searchBuffer[128] = "";
	static int  selectedIndex     = 0;


	bool matches_search(std::string name, std::string searchBuffer)
	{
		if (searchBuffer[0] != '\0') {
			// case-insensitive search
			std::string query = searchBuffer;
			std::transform(name.begin(), name.end(), name.begin(), ::tolower);
			std::transform(query.begin(), query.end(), query.begin(), ::tolower);

			return name.find(query) != std::string::npos;
		}
		return true;
	}


	void InspectorRenderer::RenderInspectorWindow(Entity* m_selectedEntityP)
	{
		ImGui::Begin("Inspector");

		if ((*m_selectedEntityP)) {
			// Display entity name at the top
			auto& metadata = (*m_selectedEntityP).GetComponent<Components::EntityMetadata>();

			ImGui::Text("guid: %s", metadata.guid.c_str());

			// Top row: active checkbox + name
			ImGui::BeginGroup();
			ImGui::PushID("isActiveTop");
			ImGui::Checkbox("", &metadata.active);
			ImGui::PopID();
			ImGui::SameLine();
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
			ImGui::InputText("##nameTop", &metadata.name);
			ImGui::Spacing();

			// Tag row with Delete button
			const char* deleteLabel = "Delete";
			ImVec2      deleteSize  = ImGui::CalcTextSize(deleteLabel);
			float       deleteWidth = deleteSize.x + ImGui::GetStyle().FramePadding.x * 2.0f;
			float       spacing     = ImGui::GetStyle().ItemSpacing.x;
			float       availWidth  = ImGui::GetContentRegionAvail().x;
			float       tagWidth    = availWidth - (deleteWidth + spacing);

			ImGui::PushItemWidth(tagWidth);
			ImGui::InputText("##Taginspector", &metadata.tag);
			ImGui::PopItemWidth();

			ImGui::SameLine(0.0f, spacing);

			// Red delete button
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.1f, 0.1f, 1.0f));

			if (ImGui::Button(deleteLabel, ImVec2(deleteWidth, 0))) {
				GetUI().m_selectedEntity.Destroy();
				GetUI().m_selectedEntity = Entity();
				ImGui::PopStyleColor(3);
				ImGui::EndGroup();
				ImGui::End();
				return;
			}

			ImGui::PopStyleColor(3);

			EntityHandle newParent = metadata.parentEntity;
			if (LeftLabelEntity("parent", &newParent)) {
				m_selectedEntityP->SetParent(newParent);
			}


			ImGui::EndGroup();

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();


			std::vector<std::function<void()>> pendingRemovals;


#define X(type, name, fancy)                                                                                                                                                                                                                   \
	if ((*m_selectedEntityP).HasComponent<type>()) {                                                                                                                                                                                           \
		ImGui::PushID(#type);                                                                                                                                                                                                                  \
		bool trash = false;                                                                                                                                                                                                                    \
		bool open  = ComponentHeader(fancy, &trash);                                                                                                                                                                                           \
		if (trash) {                                                                                                                                                                                                                           \
			auto entityPtr = m_selectedEntityP;                                                                                                                                                                                                \
			pendingRemovals.emplace_back([entityPtr]() { (*entityPtr).RemoveComponent<type>(); });                                                                                                                                             \
		}                                                                                                                                                                                                                                      \
		if (open) {                                                                                                                                                                                                                            \
			ImGui::Indent();                                                                                                                                                                                                                   \
			(*m_selectedEntityP).GetComponent<type>().RenderInspector((*m_selectedEntityP));                                                                                                                                                   \
			ImGui::Unindent();                                                                                                                                                                                                                 \
			ImGui::Spacing();                                                                                                                                                                                                                  \
		}                                                                                                                                                                                                                                      \
                                                                                                                                                                                                                                               \
                                                                                                                                                                                                                                               \
		ImGui::PopID();                                                                                                                                                                                                                        \
	}
			COMPONENT_LIST
#undef X


			for (auto& remove : pendingRemovals) {
				remove();
			}
			pendingRemovals.clear();


			ImGui::Separator();
			ImGui::Spacing();


			ImGui::Dummy(ImVec2(0.0f, 6.0f));

			float avail = ImGui::GetContentRegionAvail().x;
			float btn_w = avail * 0.9f;
			ImGui::SetCursorPosX((avail - btn_w) * 0.5f);
			if (ImGui::Button("Add Component", ImVec2(btn_w, 0))) {
				m_openPopup = true;
				ImGui::OpenPopup("Components");
			}


			// Popup definition
			if (ImGui::BeginPopupModal("Components", &m_openPopup, ImGuiWindowFlags_NoMove)) {
				// Ensure keyboard focus starts in search bar
				if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();

				// Search bar
				ImGui::PushItemWidth(-FLT_MIN);
				ImGui::InputTextWithHint("##search", ICON_FA_MAGNIFYING_GLASS " Search...", searchBuffer, IM_ARRAYSIZE(searchBuffer));
				ImGui::PopItemWidth();

				ImGui::Separator();


				// Escape closes popup
				if (ImGui::IsKeyPressed(ImGuiKey_Escape)) ImGui::CloseCurrentPopup();

				int i = 0;


#define X(type, name, fancy)                                                                                                                                                                                                                   \
	if (matches_search(#name, searchBuffer) && !(*m_selectedEntityP).HasComponent<type>()) {                                                                                                                                                   \
		bool selected = (i == selectedIndex);                                                                                                                                                                                                  \
		if (ImGui::Selectable(#name, selected, ImGuiSelectableFlags_AllowDoubleClick)) {                                                                                                                                                       \
			selectedIndex = i;                                                                                                                                                                                                                 \
			GetDefaultLogger()->info("selected {}", #name);                                                                                                                                                                                    \
			(*m_selectedEntityP).AddComponent<type>();                                                                                                                                                                                         \
			ImGui::CloseCurrentPopup();                                                                                                                                                                                                        \
		}                                                                                                                                                                                                                                      \
	}                                                                                                                                                                                                                                          \
	i++;

				COMPONENT_LIST
#undef X
				// Handle up/down key navigation
				if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) selectedIndex = (selectedIndex + 1) % i;
				if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) selectedIndex = (selectedIndex - 1 + i) % i;

				ImGui::EndPopup();
			}
		}
		else {
			ImGui::Text("No entity selected");
		}

		ImGui::End();
	}


} // namespace Engine
