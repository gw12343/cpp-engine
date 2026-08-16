//
// Created by gabe on 8/25/25.
//


#include "InspectorUI.h"
#include "misc/cpp/imgui_stdlib.h"
#include "rendering/Renderer.h"
#include <nfd.h>

#include "string"
#include "imgui_internal.h"
#include "rendering/particles/Particle.h"
#include "IconsFontAwesome6.h"
#include "animation/Animation.h"
#include "animation/Skeleton.h"
#include "rendering/Texture.h"
#include "rendering/Material.h"
#include "core/Scene.h"
#include "rendering/Model.h"
#include "terrain/TerrainTile.h"
#include "sound/SoundManager.h"
#include "core/Entity.h"
#include "assets/Prefab.h"


namespace Engine {
	namespace {
		constexpr float kLabelPad       = 10.0f;
		constexpr float kMinWidgetWidth = 112.0f;

		float CurrentLeftLabelWidth()
		{
			ImGuiWindow* window = ImGui::GetCurrentWindow();
			if (!window) {
				return kLabelPad;
			}
			return ImGui::GetStateStorage()->GetFloat(window->GetID("##LLW"), kLabelPad);
		}

		// Shared among rows in the current ID scope (component / Variables tree).
		// Width is exactly the longest label — not the remaining panel width.
		float ResolveLeftLabelWidth(const char* label, float requested)
		{
			const float avail      = ImGui::GetContentRegionAvail().x;
			const float maxAllowed = ImMax(kLabelPad, avail - kMinWidgetWidth);

			float needed = 0.0f;
			if (label && label[0] != '\0') {
				needed = ImGui::CalcTextSize(label).x + kLabelPad;
			}
			if (requested > 0.0f) {
				needed = ImMax(needed, requested);
			}

			ImGuiWindow* window = ImGui::GetCurrentWindow();
			if (!window) {
				return ImMin(needed, maxAllowed);
			}

			ImGuiStorage* st        = ImGui::GetStateStorage();
			const ImGuiID idW       = window->GetID("##LLW");
			const ImGuiID idP       = window->GetID("##LLP");
			const ImGuiID idF       = window->GetID("##LLF");
			const int     frame     = ImGui::GetFrameCount();
			const int     prevFrame = st->GetInt(idF, -1);
			if (prevFrame != frame) {
				if (prevFrame != -1) {
					st->SetFloat(idW, st->GetFloat(idP, 0.0f));
				}
				st->SetFloat(idP, 0.0f);
				st->SetInt(idF, frame);
			}

			const float pending = ImMax(st->GetFloat(idP, 0.0f), needed);
			st->SetFloat(idP, pending);

			const float committed = st->GetFloat(idW, 0.0f);
			return ImMin(ImMax(committed, pending), maxAllowed);
		}

		void BeginLeftLabelRow(const char* label, float requestedWidth = 0.0f)
		{
			// Measure before PushID so every field in this scope shares one width.
			const float w = ResolveLeftLabelWidth(label, requestedWidth);
			ImGui::PushID(label ? label : "");
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 6));

			const ImVec2 p = ImGui::GetCursorScreenPos();
			ImGui::Dummy(ImVec2(w, ImGui::GetFrameHeight()));
			ImGui::SetCursorScreenPos(p);
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted(label ? label : "");
			ImGui::SetCursorScreenPos(ImVec2(p.x + w, p.y));
			ImGui::SetNextItemWidth(ImMax(kMinWidgetWidth, ImGui::GetContentRegionAvail().x));
		}

		void EndLeftLabelRow()
		{
			ImGui::PopStyleVar();
			ImGui::PopID();
		}
	} // namespace

	bool LeftLabelCheckbox(const char* label, bool* value, float labelWidth)
	{
		BeginLeftLabelRow(label, labelWidth);
		bool changed = ImGui::Checkbox("##v", value);
		EndLeftLabelRow();
		return changed;
	}

	bool LeftLabelInputText(const char* label, char* buf, size_t buf_size, float labelWidth, ImGuiInputTextFlags flags)
	{
		BeginLeftLabelRow(label, labelWidth);
		ImGui::SetNextItemWidth(-1);
		bool changed = ImGui::InputText("##v", buf, buf_size, flags);
		EndLeftLabelRow();
		return changed;
	}

	bool LeftLabelInputText(const char* label, std::string* str, float labelWidth, ImGuiInputTextFlags flags)
	{
		BeginLeftLabelRow(label, labelWidth);
		ImGui::SetNextItemWidth(-1);
		bool changed = ImGui::InputText("##v", str, flags);
		EndLeftLabelRow();
		return changed;
	}

	bool LeftLabelDragFloat3(const char* label, float v[3], float speed, float labelWidth)
	{
		BeginLeftLabelRow(label, labelWidth);
		ImGui::SetNextItemWidth(-1);
		bool changed = ImGui::DragFloat3("##v", v, speed, -FLT_MAX, FLT_MAX, "%.3f");
		EndLeftLabelRow();
		return changed;
	}

	bool LeftLabelColorEdit3(const char* label, float col[3], ImGuiColorEditFlags flags, float labelWidth)
	{
		BeginLeftLabelRow(label, labelWidth);
		ImGui::SetNextItemWidth(-1);
		bool changed = ImGui::ColorEdit3("##color", col, flags);
		EndLeftLabelRow();
		return changed;
	}

	bool LeftLabelDragFloat2(const char* label, float v[2], float speed, float labelWidth)
	{
		BeginLeftLabelRow(label, labelWidth);
		ImGui::SetNextItemWidth(-1);
		bool changed = ImGui::DragFloat2("##v", v, speed, -FLT_MAX, FLT_MAX, "%.3f");
		EndLeftLabelRow();
		return changed;
	}

	bool LeftLabelSliderFloat(const char* label, float* v, float v_min, float v_max, const char* format, ImGuiSliderFlags flags, float labelWidth)
	{
		BeginLeftLabelRow(label, labelWidth);
		ImGui::SetNextItemWidth(-1);
		bool changed = ImGui::SliderFloat("##v", v, v_min, v_max, format, flags);
		EndLeftLabelRow();
		return changed;
	}

	bool LeftLabelDragFloat(const char* label, float* v, float speed, float labelWidth)
	{
		BeginLeftLabelRow(label, labelWidth);
		ImGui::SetNextItemWidth(-1);
		bool changed = ImGui::DragFloat("##v", v, speed, -FLT_MAX, FLT_MAX, "%.3f");
		EndLeftLabelRow();
		return changed;
	}

	bool LeftLabelCombo(const char* label, int* currentItem, const char* const items[], int itemsCount, float labelWidth)
	{
		BeginLeftLabelRow(label, labelWidth);
		ImGui::SetNextItemWidth(-1);
		bool changed = ImGui::Combo("##v", currentItem, items, itemsCount);
		EndLeftLabelRow();
		return changed;
	}

	bool LeftLabelBeginCombo(const char* label, const char* preview_value, ImGuiComboFlags flags, float labelWidth)
	{
		BeginLeftLabelRow(label, labelWidth);
		ImGui::SetNextItemWidth(-1);
		bool opened = ImGui::BeginCombo("##combo", preview_value, flags);
		if (!opened) {
			EndLeftLabelRow();
		}
		return opened;
	}

	void LeftLabelEndCombo()
	{
		ImGui::EndCombo();
		EndLeftLabelRow();
	}

	template <typename T, typename EditorFunc>
	bool LeftLabelEditVector(const char* label, std::vector<T>& vec, EditorFunc editor)
	{
		bool is_using = false;
		if (ImGui::CollapsingHeader(label, ImGuiTreeNodeFlags_DefaultOpen)) {
			// Begin scrollable frame
			ImGui::BeginChild((std::string("##") + label).c_str(), ImVec2(0, 200), true);

			for (size_t i = 0; i < vec.size(); ++i) {
				ImGui::PushID(static_cast<int>(i)); // ensure unique IDs

				ImGui::Separator();

				// Element label + inline remove button
				ImGui::Text("Element %zu", i);
				ImGui::SameLine();
				if (ImGui::Button(("-##VECTORDELETEBUTTON" + std::string(label)).c_str())) {
					vec.erase(vec.begin() + i);
					ImGui::PopID();
					is_using = true;
					break; // exit loop after modifying container
				}

				// Call user-supplied editor for this element
				if (editor(vec[i])) {
					is_using = true;
				}

				ImGui::PopID();
			}

			ImGui::EndChild();

			// Add new element
			if (ImGui::Button(("+##VECTORADDBUTTON" + std::string(label)).c_str())) {
				vec.push_back(T{}); // default-construct new element
				is_using = true;
			}
		}

		return is_using;
	}


#define LL_ASSET_DEF(name, atype, an, nameA)                                                                                                                                                                                                   \
	bool LeftLabelAsset##name(const char* label, AssetHandle<atype>* assetRef)                                                                                                                                                                 \
	{                                                                                                                                                                                                                                          \
		bool used = false;                                                                                                                                                                                                                     \
		BeginLeftLabelRow(label);                                                                                                                                                                                                              \
		std::string preview = "None";                                                                                                                                                                                                          \
		bool        valid   = false;                                                                                                                                                                                                           \
		if (assetRef->IsValid()) {                                                                                                                                                                                                             \
			atype* assetPtr = GetAssetManager().Get(*assetRef);                                                                                                                                                                                \
			if (assetPtr != nullptr) {                                                                                                                                                                                                         \
				preview = nameA;                                                                                                                                                                                                               \
				valid   = true;                                                                                                                                                                                                                \
			}                                                                                                                                                                                                                                  \
			else {                                                                                                                                                                                                                             \
				preview = "Missing";                                                                                                                                                                                                           \
			}                                                                                                                                                                                                                                  \
		}                                                                                                                                                                                                                                      \
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 52.0f);                                                                                                                                                                     \
		ImGui::PushStyleColor(ImGuiCol_Text, valid ? ImVec4(0.5f, 1.f, 0.5f, 1.f) : ImVec4(1.f, 0.85f, 0.3f, 1.f));                                                                                                                            \
		ImGui::InputText("##id", &preview, ImGuiInputTextFlags_ReadOnly);                                                                                                                                                                      \
		ImGui::PopStyleColor();                                                                                                                                                                                                                \
		if (ImGui::BeginDragDropTarget()) {                                                                                                                                                                                                    \
			struct PayloadData {                                                                                                                                                                                                               \
				const char* type;                                                                                                                                                                                                              \
				char        id[64];                                                                                                                                                                                                            \
			};                                                                                                                                                                                                                                 \
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(an)) {                                                                                                                                                              \
				if (payload->DataSize == sizeof(PayloadData)) {                                                                                                                                                                                \
					const PayloadData* data = static_cast<const PayloadData*>(payload->Data);                                                                                                                                                  \
					if (std::strcmp(data->type, #atype) == 0) {                                                                                                                                                                                \
						*assetRef = AssetHandle<atype>(data->id);                                                                                                                                                                              \
						used      = true;                                                                                                                                                                                                      \
					}                                                                                                                                                                                                                          \
				}                                                                                                                                                                                                                              \
			}                                                                                                                                                                                                                                  \
			ImGui::EndDragDropTarget();                                                                                                                                                                                                        \
		}                                                                                                                                                                                                                                      \
		ImGui::SameLine();                                                                                                                                                                                                                     \
		if (ImGui::Button("...")) {                                                                                                                                                                                                            \
			ImGui::OpenPopup("##asset_browse");                                                                                                                                                                                                \
		}                                                                                                                                                                                                                                      \
		ImGui::SameLine();                                                                                                                                                                                                                     \
		if (ImGui::Button("X")) {                                                                                                                                                                                                              \
			*assetRef = AssetHandle<atype>();                                                                                                                                                                                                  \
			used      = true;                                                                                                                                                                                                                  \
		}                                                                                                                                                                                                                                      \
		if (ImGui::BeginPopup("##asset_browse")) {                                                                                                                                                                                              \
			static char filter[128] = "";                                                                                                                                                                                                      \
			ImGui::InputTextWithHint("##af", "Filter...", filter, IM_ARRAYSIZE(filter));                                                                                                                                                       \
			auto& storage = GetAssetManager().GetStorage<atype>();                                                                                                                                                                             \
			ImGui::BeginChild("##alist", ImVec2(280, 220), true);                                                                                                                                                                              \
			for (const auto& [guid, asset] : storage.guidToAsset) {                                                                                                                                                                            \
				if (!asset) continue;                                                                                                                                                                                                          \
				AssetHandle<atype> h(guid);                                                                                                                                                                                                    \
				atype*             assetPtr = asset.get();                                                                                                                                                                                     \
				std::string        shown    = nameA;                                                                                                                                                                                           \
				if (filter[0] && shown.find(filter) == std::string::npos) continue;                                                                                                                                                            \
				if (ImGui::Selectable(shown.c_str())) {                                                                                                                                                                                        \
					*assetRef = h;                                                                                                                                                                                                             \
					used      = true;                                                                                                                                                                                                          \
					ImGui::CloseCurrentPopup();                                                                                                                                                                                                \
				}                                                                                                                                                                                                                              \
			}                                                                                                                                                                                                                                  \
			ImGui::EndChild();                                                                                                                                                                                                                 \
			ImGui::EndPopup();                                                                                                                                                                                                                 \
		}                                                                                                                                                                                                                                      \
		EndLeftLabelRow();                                                                                                                                                                                                                     \
		return used;                                                                                                                                                                                                                           \
	} \
	bool LeftLabelAssetVector##name(const char* label, std::vector<AssetHandle<atype>>& assetRef)                                                                                                                                              \
	{                                                                                                                                                                                                                                          \
		if (LeftLabelEditVector<AssetHandle<atype>>(label, assetRef, [](AssetHandle<atype>& val) {                                                                                                                                             \
			    ImGui::SameLine();                                                                                                                                                                                                             \
			    return LeftLabelAsset##name("", &val);                                                                                                                                                                                         \
		    })) {                                                                                                                                                                                                                              \
			return true;                                                                                                                                                                                                                       \
		}                                                                                                                                                                                                                                      \
		return false;                                                                                                                                                                                                                          \
	}

	LL_ASSET_DEF(Texture, Texture, "ASSET_TEXTURE", assetPtr->GetName().c_str())
	LL_ASSET_DEF(Model, Rendering::Model, "ASSET_MODEL", assetPtr->m_name.c_str())
	LL_ASSET_DEF(Terrain, Terrain::TerrainTile, "ASSET_TERRAIN", assetRef->GetID().c_str())
	LL_ASSET_DEF(Sound, Audio::SoundBuffer, "ASSET_SOUND", assetPtr->name.c_str())
	LL_ASSET_DEF(Scene, Scene, "ASSET_SCENE", assetPtr->GetName().c_str())
	LL_ASSET_DEF(Particle, Particle, "ASSET_PARTICLE", assetPtr->name.c_str())
	LL_ASSET_DEF(Material, Material, "ASSET_MATERIAL", assetPtr->GetName().c_str())
	LL_ASSET_DEF(Animation, Animation, "ASSET_ANIMATION", assetPtr->name.c_str())
	LL_ASSET_DEF(Skeleton, Skeleton, "ASSET_SKELETON", assetPtr->name.c_str())
	LL_ASSET_DEF(Prefab, Prefab, "ASSET_PREFAB", assetPtr->GetName().c_str())


	bool LeftLabelEntity(const char* label, EntityHandle* assetRef)
	{
		bool        used  = false;
		std::string newID = assetRef->GetID();
		if (LeftLabelInputText(label, &newID)) {
			*assetRef = EntityHandle(newID);
			used      = true;
		}


		if (ImGui::BeginDragDropTarget()) {
			struct PayloadData {
				const char* type;
				char        id[64];
			};
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY_HANDLE")) {
				if (payload->DataSize == sizeof(PayloadData)) {
					const auto* data = static_cast<const PayloadData*>(payload->Data);
					if (std::strcmp(data->type, "EntityHandle") == 0) {
						*assetRef = EntityHandle(data->id);
						newID     = data->id;
						used      = true;
					}
				}
			}
			ImGui::EndDragDropTarget();
		}

		ImGui::Indent(CurrentLeftLabelWidth());
		bool drawDefault = false;
		if (assetRef->IsValid()) {
			Entity assetPtr = GetCurrentScene()->Get(*assetRef);
			if (assetPtr.IsValid()) {
				ImGui::PushStyleColor(ImGuiCol_Text, (((ImU32) (255) << 24) | ((ImU32) (0) << 16) | ((ImU32) (255) << 8) | ((ImU32) (0) << 0)));
				ImGui::Text("^^^ Entity: %s", assetPtr.GetName().c_str());
				ImGui::PopStyleColor();
			}
			else {
				drawDefault = true;
			}
		}
		else {
			drawDefault = true;
		}
		if (drawDefault) {
			ImGui::PushStyleColor(ImGuiCol_Text, (((ImU32) (255) << 24) | ((ImU32) (0) << 16) | ((ImU32) (255) << 8) | ((ImU32) (255) << 0)));
			ImGui::Text("^^^ Invalid Entity");
			ImGui::PopStyleColor();
		}
		ImGui::Unindent(CurrentLeftLabelWidth());

		return used;
	}

	bool LeftLabelEntityVector(const char* label, std::vector<EntityHandle>& assetRef)
	{
		if (LeftLabelEditVector<EntityHandle>(label, assetRef, [](EntityHandle& val) {
			    ImGui::SameLine();
			    return LeftLabelEntity("", &val);
		    })) {
			return true;
		}
		return false;
	}


	bool BrowsePathButton(const char* id, const char* filter, const char* defaultPath, std::string* path)
	{
		ImGui::PushID(id);
		bool changed = false;
		if (ImGui::Button("Browse...")) {
			nfdchar_t*  outPath = nullptr;
			nfdresult_t result  = NFD_OpenDialog(filter, defaultPath, &outPath);
			if (result == NFD_OKAY && outPath) {
				*path = outPath;
				free(outPath);
				changed = true;
			}
		}
		ImGui::PopID();
		return changed;
	}

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
