//
// Created by gabe on 8/21/25.
//

#include "InspectorRenderer.h"
#include "imgui.h"
#include "components/AllComponents.h"
#include "rendering/ui/IconsFontAwesome6.h"

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
			ImGui::Text("Selected: %s", metadata.name.c_str());
			ImGui::SameLine();
			ImVec4 myColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // bluish

			ImGui::PushStyleColor(ImGuiCol_Button, myColor);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(myColor.x * 1.1f, myColor.y * 1.1f, myColor.z * 1.1f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(myColor.x * 0.9f, myColor.y * 0.9f, myColor.z * 0.9f, 1.0f));

			if (ImGui::Button(ICON_FA_TRASH_CAN "", ImVec2(100, 40))) {
				(*m_selectedEntityP).Destroy();
				if (GetCurrentSceneRegistry().valid((*m_selectedEntityP).GetHandle())) GetCurrentSceneRegistry().destroy((*m_selectedEntityP).GetHandle());
				ImGui::PopStyleColor(3);
				ImGui::End();
				(*m_selectedEntityP) = Entity();
				return;
			}


			ImGui::PopStyleColor(3);

			ImGui::Separator();

			// Render each component in the inspector
			if ((*m_selectedEntityP).HasComponent<Components::EntityMetadata>()) {
				if (ImGui::CollapsingHeader(ICON_FA_ID_CARD " Entity Metadata", ImGuiTreeNodeFlags_DefaultOpen)) {
					(*m_selectedEntityP).GetComponent<Components::EntityMetadata>().RenderInspector((*m_selectedEntityP));
				}
			}

			//#define X(type, name, fancy)                                                                                                                                                                                                                   \
//	if ((*m_selectedEntityP).HasComponent<type>()) {                                                                                                                                                                                           \
//		if (ImGui::CollapsingHeader(fancy)) {                                                                                                                                                                                                  \
//			(*m_selectedEntityP).GetComponent<type>().RenderInspector((*m_selectedEntityP));                                                                                                                                                   \
//		}                                                                                                                                                                                                                                      \
//	}
			//			COMPONENT_LIST
			// #undef X


			std::vector<std::function<void()>> pendingRemovals;


#define X(type, name, fancy)                                                                                                                                                                                                                   \
	if ((*m_selectedEntityP).HasComponent<type>()) {                                                                                                                                                                                           \
		ImGui::PushID(#type);                                                                                                                                                                                                                  \
                                                                                                                                                                                                                                               \
		if (ImGui::BeginTable("compHeader", 2, ImGuiTableFlags_SizingStretchSame)) {                                                                                                                                                           \
			ImGui::TableNextRow();                                                                                                                                                                                                             \
                                                                                                                                                                                                                                               \
			/* Header cell */                                                                                                                                                                                                                  \
			ImGui::TableSetColumnIndex(0);                                                                                                                                                                                                     \
			bool open = ImGui::CollapsingHeader(fancy, ImGuiTreeNodeFlags_DefaultOpen);                                                                                                                                                        \
                                                                                                                                                                                                                                               \
			/* Trashcan cell */                                                                                                                                                                                                                \
			ImGui::TableSetColumnIndex(1);                                                                                                                                                                                                     \
			float buttonSize = ImGui::GetFrameHeight();                                                                                                                                                                                        \
			if (ImGui::Button(ICON_FA_TRASH_CAN, ImVec2(buttonSize, buttonSize))) {                                                                                                                                                            \
				auto entityPtr = m_selectedEntityP;                                                                                                                                                                                            \
				pendingRemovals.push_back([entityPtr]() { (*entityPtr).RemoveComponent<type>(); });                                                                                                                                            \
			}                                                                                                                                                                                                                                  \
                                                                                                                                                                                                                                               \
			ImGui::EndTable();                                                                                                                                                                                                                 \
                                                                                                                                                                                                                                               \
			if (open) {                                                                                                                                                                                                                        \
				(*m_selectedEntityP).GetComponent<type>().RenderInspector((*m_selectedEntityP));                                                                                                                                               \
			}                                                                                                                                                                                                                                  \
		}                                                                                                                                                                                                                                      \
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
			if (ImGui::Button("Add Component " ICON_FA_PLUS, ImVec2(-FLT_MIN, 0))) {
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