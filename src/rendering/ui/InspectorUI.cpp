//
// Created by gabe on 8/25/25.
//

#include "InspectorUI.h"
#include "misc/cpp/imgui_stdlib.h"
#include "rendering/Renderer.h"
#include "assets/AssetManager.h"
#include "string"
#include "imgui_internal.h"
#include "rendering/particles/Particle.h"
#include "IconsFontAwesome6.h"


namespace Engine {
	// Reusable helpers (unchanged, but included for completeness)
	bool LeftLabelCheckbox(const char* label, bool* value, float labelWidth)
	{
		ImGui::PushID(label);
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 6));
		ImGui::Columns(2, nullptr, false);
		ImGui::SetColumnWidth(0, labelWidth);
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(label);
		ImGui::NextColumn();

		bool changed = ImGui::Checkbox("", value);

		ImGui::Columns(1);
		ImGui::PopStyleVar();
		ImGui::PopID();
		return changed;
	}

	bool LeftLabelInputText(const char* label, char* buf, size_t buf_size, float labelWidth, ImGuiInputTextFlags flags)
	{
		ImGui::PushID(label);
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 6));
		ImGui::Columns(2, nullptr, false);
		ImGui::SetColumnWidth(0, labelWidth);
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(label);
		ImGui::NextColumn();

		ImGui::SetNextItemWidth(-1); // use remaining width
		bool changed = ImGui::InputText("", buf, buf_size, flags);

		ImGui::Columns(1);
		ImGui::PopStyleVar();
		ImGui::PopID();
		return changed;
	}

	bool LeftLabelInputText(const char* label, std::string* str, float labelWidth, ImGuiInputTextFlags flags)
	{
		ImGui::PushID(label);
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 6));
		ImGui::Columns(2, nullptr, false);
		ImGui::SetColumnWidth(0, labelWidth);
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(label);
		ImGui::NextColumn();

		ImGui::SetNextItemWidth(-1); // use remaining width
		bool changed = ImGui::InputText("", str, flags);

		ImGui::Columns(1);
		ImGui::PopStyleVar();
		ImGui::PopID();
		return changed;
	}

	bool LeftLabelDragFloat3(const char* label, float v[3], float speed, float labelWidth)
	{
		ImGui::PushID(label);
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 6));
		ImGui::Columns(2, nullptr, false);
		ImGui::SetColumnWidth(0, labelWidth);
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(label);
		ImGui::NextColumn();

		ImGui::SetNextItemWidth(-1);
		bool changed = ImGui::DragFloat3("", v, speed, -FLT_MAX, FLT_MAX, "%.3f");

		ImGui::Columns(1);
		ImGui::PopStyleVar();
		ImGui::PopID();
		return changed;
	}

	bool LeftLabelColorEdit3(const char* label, float col[3], ImGuiColorEditFlags flags, float labelWidth)
	{
		ImGui::PushID(label);
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 6));
		ImGui::Columns(2, nullptr, false);
		ImGui::SetColumnWidth(0, labelWidth);
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(label);
		ImGui::NextColumn();

		ImGui::SetNextItemWidth(-1);
		bool changed = ImGui::ColorEdit3("##color", col, flags);

		ImGui::Columns(1);
		ImGui::PopStyleVar();
		ImGui::PopID();
		return changed;
	}

	bool LeftLabelDragFloat2(const char* label, float v[2], float speed, float labelWidth)
	{
		ImGui::PushID(label);
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 6));
		ImGui::Columns(2, nullptr, false);
		ImGui::SetColumnWidth(0, labelWidth);
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(label);
		ImGui::NextColumn();

		ImGui::SetNextItemWidth(-1);
		bool changed = ImGui::DragFloat2("", v, speed, -FLT_MAX, FLT_MAX, "%.3f");

		ImGui::Columns(1);
		ImGui::PopStyleVar();
		ImGui::PopID();
		return changed;
	}

	bool LeftLabelSliderFloat(const char* label, float* v, float v_min, float v_max, const char* format, ImGuiSliderFlags flags, float labelWidth)
	{
		ImGui::PushID(label);
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 6));
		ImGui::Columns(2, nullptr, false);
		ImGui::SetColumnWidth(0, labelWidth);
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(label);
		ImGui::NextColumn();

		ImGui::SetNextItemWidth(-1);
		bool changed = ImGui::SliderFloat("", v, v_min, v_max, format, flags);

		ImGui::Columns(1);
		ImGui::PopStyleVar();
		ImGui::PopID();
		return changed;
	}

	bool LeftLabelDragFloat(const char* label, float* v, float speed, float labelWidth)
	{
		ImGui::PushID(label);
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 6));
		ImGui::Columns(2, nullptr, false);
		ImGui::SetColumnWidth(0, labelWidth);
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(label);
		ImGui::NextColumn();

		ImGui::SetNextItemWidth(-1);
		bool changed = ImGui::DragFloat("", v, speed, -FLT_MAX, FLT_MAX, "%.3f");

		ImGui::Columns(1);
		ImGui::PopStyleVar();
		ImGui::PopID();
		return changed;
	}

	bool LeftLabelCombo(const char* label, int* currentItem, const char* const items[], int itemsCount, float labelWidth)
	{
		ImGui::PushID(label);
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 6));

		ImGui::Columns(2, nullptr, false);
		ImGui::SetColumnWidth(0, labelWidth);
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(label);
		ImGui::NextColumn();

		ImGui::SetNextItemWidth(-1);
		bool changed = ImGui::Combo("", currentItem, items, itemsCount);

		ImGui::Columns(1);
		ImGui::PopStyleVar();
		ImGui::PopID();
		return changed;
	}


	bool LeftLabelBeginCombo(const char* label, const char* preview_value, ImGuiComboFlags flags, float labelWidth)
	{
		ImGui::PushID(label);
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 6));

		ImGui::Columns(2, nullptr, false);
		ImGui::SetColumnWidth(0, labelWidth);
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(label);
		ImGui::NextColumn();

		ImGui::SetNextItemWidth(-1);
		bool opened = ImGui::BeginCombo("##combo", preview_value, flags);

		// Always clean up regardless of whether combo opened
		if (!opened) {
			ImGui::NextColumn();
			ImGui::Columns(1);
			ImGui::PopStyleVar();
			ImGui::PopID();
		}

		return opened;
	}

	void LeftLabelEndCombo()
	{
		ImGui::EndCombo();
		ImGui::NextColumn(); // Move back to first column
		ImGui::Columns(1);   // End column layout
		ImGui::PopStyleVar();
		ImGui::PopID();
	}


#define LL_ASSET_DEF(name, atype, an, nameA)                                                                                                                                                                                                   \
	bool LeftLabelAsset##name(const char* label, AssetHandle<atype>* assetRef)                                                                                                                                                                 \
	{                                                                                                                                                                                                                                          \
		bool        used  = false;                                                                                                                                                                                                             \
		std::string newID = assetRef->GetID();                                                                                                                                                                                                 \
		if (LeftLabelInputText(label, &newID)) {                                                                                                                                                                                               \
			*assetRef = AssetHandle<atype>(newID);                                                                                                                                                                                             \
			used      = true;                                                                                                                                                                                                                  \
		}                                                                                                                                                                                                                                      \
		ImGui::Indent(120);                                                                                                                                                                                                                    \
		bool drawDefault = false;                                                                                                                                                                                                              \
		if (assetRef->IsValid()) {                                                                                                                                                                                                             \
			atype* assetPtr = GetAssetManager().Get(*assetRef);                                                                                                                                                                                \
			if (assetPtr != nullptr) {                                                                                                                                                                                                         \
				ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 255, 0, 255));                                                                                                                                                                \
				ImGui::Text("^^^ %s: %s", #name, nameA);                                                                                                                                                                                       \
				ImGui::PopStyleColor();                                                                                                                                                                                                        \
			}                                                                                                                                                                                                                                  \
			else {                                                                                                                                                                                                                             \
				drawDefault = true;                                                                                                                                                                                                            \
			}                                                                                                                                                                                                                                  \
		}                                                                                                                                                                                                                                      \
		else {                                                                                                                                                                                                                                 \
			drawDefault = true;                                                                                                                                                                                                                \
		}                                                                                                                                                                                                                                      \
		if (drawDefault) {                                                                                                                                                                                                                     \
			ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 0, 255));                                                                                                                                                                  \
			ImGui::Text("^^^ Invalid %s", #name);                                                                                                                                                                                              \
			ImGui::PopStyleColor();                                                                                                                                                                                                            \
		}                                                                                                                                                                                                                                      \
		ImGui::Unindent(120);                                                                                                                                                                                                                  \
                                                                                                                                                                                                                                               \
                                                                                                                                                                                                                                               \
		if (ImGui::BeginDragDropTarget()) {                                                                                                                                                                                                    \
			struct PayloadData {                                                                                                                                                                                                               \
				const char* type;                                                                                                                                                                                                              \
				char        id[64];                                                                                                                                                                                                            \
			};                                                                                                                                                                                                                                 \
                                                                                                                                                                                                                                               \
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(an)) {                                                                                                                                                              \
				if (payload->DataSize == sizeof(PayloadData)) {                                                                                                                                                                                \
					const PayloadData* data = static_cast<const PayloadData*>(payload->Data);                                                                                                                                                  \
                                                                                                                                                                                                                                               \
					if (std::strcmp(data->type, #name) == 0) {                                                                                                                                                                                 \
						*assetRef = AssetHandle<atype>(data->id);                                                                                                                                                                              \
						newID     = data->id;                                                                                                                                                                                                  \
						used      = true;                                                                                                                                                                                                      \
					}                                                                                                                                                                                                                          \
				}                                                                                                                                                                                                                              \
			}                                                                                                                                                                                                                                  \
			ImGui::EndDragDropTarget();                                                                                                                                                                                                        \
		}                                                                                                                                                                                                                                      \
		return used;                                                                                                                                                                                                                           \
	}


	LL_ASSET_DEF(Texture, Texture, "ASSET_TEXTURE", assetPtr->GetName().c_str())
	LL_ASSET_DEF(Model, Rendering::Model, "ASSET_MODEL", assetPtr->m_name.c_str())
	LL_ASSET_DEF(Terrain, Terrain::TerrainTile, "ASSET_TERRAIN", assetRef->GetID().c_str())
	LL_ASSET_DEF(Sound, Audio::SoundBuffer, "ASSET_SOUND", assetPtr->name.c_str())
	LL_ASSET_DEF(Scene, Scene, "ASSET_SCENE", assetPtr->GetName().c_str())
	LL_ASSET_DEF(Particle, Particle, "ASSET_PARTICLE", assetPtr->name.c_str())
	LL_ASSET_DEF(Material, Material, "ASSET_MATERIAL", assetPtr->GetName().c_str())


	bool ComponentHeader(const char* name, bool* removeRequested)
	{
		ImGuiStyle&  style  = ImGui::GetStyle();
		ImDrawList*  draw   = ImGui::GetWindowDrawList();
		ImGuiWindow* window = ImGui::GetCurrentWindow();

		// Calculate button dimensions
		const char* btnLabel = ICON_FA_TRASH;
		ImVec2      btnSize  = ImGui::CalcTextSize(btnLabel);
		float       btnW     = btnSize.x + style.FramePadding.x * 2.0f;
		float       btnH     = ImGui::GetFrameHeight();

		// Ensure minimum button width
		if (btnW < 20.0f) btnW = 20.0f;

		float spacing = style.ItemSpacing.x;
		float availW  = ImGui::GetContentRegionAvail().x;
		float headerW = availW - (btnW + spacing);
		float headerH = ImGui::GetFrameHeight();

		// Store/retrieve open state
		ImGuiID       id      = window->GetID(name);
		ImGuiStorage* storage = ImGui::GetStateStorage();
		bool          open    = storage->GetBool(id, true);

		// Create invisible button for header interaction
		ImVec2 headerPos = ImGui::GetCursorScreenPos();
		ImGui::InvisibleButton((std::string("header_") + name).c_str(), ImVec2(headerW, headerH));

		if (ImGui::IsItemClicked()) {
			open = !open;
			storage->SetBool(id, open);
		}

		// Draw header background
		ImU32  colBg     = ImGui::GetColorU32(open ? ImGuiCol_Header : ImGuiCol_HeaderHovered);
		ImU32  colBorder = ImGui::GetColorU32(ImGuiCol_Border);
		ImVec2 headerEnd = ImVec2(headerPos.x + headerW, headerPos.y + headerH);

		draw->AddRectFilled(headerPos, headerEnd, colBg, style.FrameRounding);
		draw->AddRect(headerPos, headerEnd, colBorder, style.FrameRounding);

		// Draw collapse arrow
		float  arrowSize   = headerH * 0.35f;
		ImVec2 arrowCenter = ImVec2(headerPos.x + style.FramePadding.x + arrowSize * 0.5f, headerPos.y + headerH * 0.5f);
		ImU32  arrowCol    = ImGui::GetColorU32(ImGuiCol_Text);

		if (open) {
			// Down-pointing triangle
			ImVec2 p1 = ImVec2(arrowCenter.x - arrowSize * 0.5f, arrowCenter.y - arrowSize * 0.25f);
			ImVec2 p2 = ImVec2(arrowCenter.x + arrowSize * 0.5f, arrowCenter.y - arrowSize * 0.25f);
			ImVec2 p3 = ImVec2(arrowCenter.x, arrowCenter.y + arrowSize * 0.5f);
			draw->AddTriangleFilled(p1, p2, p3, arrowCol);
		}
		else {
			// Right-pointing triangle
			ImVec2 p1 = ImVec2(arrowCenter.x - arrowSize * 0.25f, arrowCenter.y - arrowSize * 0.5f);
			ImVec2 p2 = ImVec2(arrowCenter.x - arrowSize * 0.25f, arrowCenter.y + arrowSize * 0.5f);
			ImVec2 p3 = ImVec2(arrowCenter.x + arrowSize * 0.5f, arrowCenter.y);
			draw->AddTriangleFilled(p1, p2, p3, arrowCol);
		}

		// Draw component name text
		float  fontSize = ImGui::GetFontSize();
		ImVec2 textPos  = ImVec2(headerPos.x + style.FramePadding.x + arrowSize + style.ItemInnerSpacing.x, headerPos.y + (headerH - fontSize) * 0.5f);
		draw->AddText(textPos, ImGui::GetColorU32(ImGuiCol_Text), name);

		// Position trash button on same line, vertically centered
		ImVec2 btnPos = ImVec2(headerEnd.x + spacing, headerPos.y);
		ImGui::SetCursorScreenPos(btnPos);

		// Use regular Button with custom size for better alignment control
		if (ImGui::Button(btnLabel, ImVec2(btnW, btnH))) {
			*removeRequested = true;
		}

		return open;
	}
} // namespace Engine
