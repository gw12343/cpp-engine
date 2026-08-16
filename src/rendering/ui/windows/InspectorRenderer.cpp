//
// Created by gabe on 8/21/25.
//

#include "InspectorRenderer.h"

#include "components/AllComponents.h"
#include "rendering/ui/UIManager.h"
#include "rendering/ui/EditorSession.h"
#include "rendering/ui/IconsFontAwesome6.h"
#include "rendering/ui/InspectorUI.h"
#include "imgui_internal.h"

#include "misc/cpp/imgui_stdlib.h"
#include <cstring>
#include <functional>
#include <vector>


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
			auto& metadata = (*m_selectedEntityP).GetComponent<Components::EntityMetadata>();

			ImGui::BeginGroup();
			ImGui::PushID("isActiveTop");
			ImGui::Checkbox("Active", &metadata.active);
			ImGui::PopID();
			ImGui::SameLine();
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
			ImGui::InputText("##nameTop", &metadata.name);

			LeftLabelInputText("Tag", &metadata.tag);
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Optional tag / layer label");

			EntityHandle newParent = metadata.parentEntity;
			if (LeftLabelEntity("Parent", &newParent)) {
				m_selectedEntityP->SetParent(newParent);
				UI::GetEditor().MarkDirty();
			}

			if (ImGui::SmallButton("Copy GUID")) {
				ImGui::SetClipboardText(metadata.guid.c_str());
			}
			ImGui::SameLine();
			ImGui::TextDisabled("%s", metadata.guid.c_str());

			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
			if (ImGui::Button("Delete Entity")) {
				UI::GetEditor().pendingDeleteEntity  = *m_selectedEntityP;
				UI::GetEditor().pendingConfirmDelete = true;
			}
			ImGui::PopStyleColor();

			ImGui::EndGroup();

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();


			static std::function<void()> pendingRemove;
			static bool                  confirmRemove = false;

			std::vector<std::function<void()>> pendingRemovals;


#define X(type, name, fancy)                                                                                                                                                                                                                   \
	if ((*m_selectedEntityP).HasComponent<type>()) {                                                                                                                                                                                           \
		ImGui::PushID(#type);                                                                                                                                                                                                                  \
		bool trash = false;                                                                                                                                                                                                                    \
		bool open  = ComponentHeader(fancy, &trash);                                                                                                                                                                                           \
		if (trash) {                                                                                                                                                                                                                           \
			auto entityPtr = m_selectedEntityP;                                                                                                                                                                                                \
			pendingRemove  = [entityPtr]() { (*entityPtr).RemoveComponent<type>(); };                                                                                                                                                          \
			confirmRemove  = true;                                                                                                                                                                                                             \
		}                                                                                                                                                                                                                                      \
		if (open) {                                                                                                                                                                                                                            \
			ImGui::Indent();                                                                                                                                                                                                                   \
			(*m_selectedEntityP).GetComponent<type>().RenderInspector((*m_selectedEntityP));                                                                                                                                                   \
			ImGui::Unindent();                                                                                                                                                                                                                 \
			ImGui::Spacing();                                                                                                                                                                                                                  \
		}                                                                                                                                                                                                                                      \
		ImGui::PopID();                                                                                                                                                                                                                        \
	}
			COMPONENT_LIST
#undef X

			if (confirmRemove) {
				ImGui::OpenPopup("Remove Component##confirm");
				confirmRemove = false;
			}
			if (ImGui::BeginPopupModal("Remove Component##confirm", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
				ImGui::TextUnformatted("Remove this component?");
				if (ImGui::Button("Remove", ImVec2(120, 0))) {
					if (pendingRemove) pendingRemove();
					pendingRemove = nullptr;
					UI::GetEditor().MarkDirty();
					ImGui::CloseCurrentPopup();
				}
				ImGui::SameLine();
				if (ImGui::Button("Cancel", ImVec2(120, 0))) {
					pendingRemove = nullptr;
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}


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

				struct CompEntry {
					const char* display;
					int         index;
				};
				static std::vector<CompEntry> visible;
				visible.clear();
				int raw = 0;
#define X(type, name, fancy)                                                                                                                                                                                                                   \
				if (matches_search(fancy, searchBuffer) && !(*m_selectedEntityP).HasComponent<type>()) {                                                                                                                                       \
					visible.push_back({fancy, raw});                                                                                                                                                                                           \
				}                                                                                                                                                                                                                              \
				++raw;
				COMPONENT_LIST
#undef X

				if (selectedIndex >= (int) visible.size()) selectedIndex = 0;

				if (ImGui::IsKeyPressed(ImGuiKey_DownArrow) && !visible.empty()) {
					selectedIndex = (selectedIndex + 1) % (int) visible.size();
				}
				if (ImGui::IsKeyPressed(ImGuiKey_UpArrow) && !visible.empty()) {
					selectedIndex = (selectedIndex - 1 + (int) visible.size()) % (int) visible.size();
				}

				auto addAt = [&](int visIndex) {
#define X(type, name, fancy)                                                                                                                                                                                                                   \
					if (visible[visIndex].display == (const char*) fancy || std::strcmp(visible[visIndex].display, fancy) == 0) {                                                                                                              \
						(*m_selectedEntityP).AddComponent<type>();                                                                                                                                                                             \
						UI::GetEditor().MarkDirty();                                                                                                                                                                                           \
						ImGui::CloseCurrentPopup();                                                                                                                                                                                            \
						return;                                                                                                                                                                                                                \
					}
					COMPONENT_LIST
#undef X
				};

				for (int i = 0; i < (int) visible.size(); ++i) {
					bool selected = (i == selectedIndex);
					if (ImGui::Selectable(visible[i].display, selected, ImGuiSelectableFlags_AllowDoubleClick)) {
						selectedIndex = i;
						addAt(i);
					}
					if (selected) ImGui::SetItemDefaultFocus();
				}

				if (ImGui::IsKeyPressed(ImGuiKey_Enter) && !visible.empty()) {
					addAt(selectedIndex);
				}

				ImGui::EndPopup();
			}
		}
		else {
			ImGui::Text("No entity selected");
		}

		ImGui::End();
	}


} // namespace Engine
