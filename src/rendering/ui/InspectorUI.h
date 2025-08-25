//
// Created by gabe on 8/25/25.
//

#ifndef CPP_ENGINE_INSPECTORUI_H
#define CPP_ENGINE_INSPECTORUI_H

#include "imgui.h"
#include "assets/AssetHandle.h"
#include <string>

namespace Engine {

	namespace Rendering {
		class Model;
	}

	class Material;
	class Texture;

	// Reusable helpers (unchanged, but included for completeness)
	bool LeftLabelCheckbox(const char* label, bool* value, float labelWidth = 100.0f);

	bool LeftLabelInputText(const char* label, char* buf, size_t buf_size, float labelWidth = 100.0f, ImGuiInputTextFlags flags = 0);
	bool LeftLabelInputText(const char* label, std::string* str, float labelWidth = 100.0f, ImGuiInputTextFlags flags = 0);

	bool LeftLabelSliderFloat(const char* label, float* v, float v_min, float v_max, const char* format = "%.3f", ImGuiSliderFlags flags = 0, float labelWidth = 100.0f);
	bool LeftLabelDragFloat(const char* label, float* v, float speed = 0.1f, float labelWidth = 100.0f);
	bool LeftLabelDragFloat2(const char* label, float v[2], float speed = 0.1f, float labelWidth = 100.0f);
	bool LeftLabelDragFloat3(const char* label, float v[3], float speed = 0.1f, float labelWidth = 100.0f);

	bool LeftLabelColorEdit3(const char* label, float col[3], ImGuiColorEditFlags flags = 0, float labelWidth = 100.0f);

	bool LeftLabelCombo(const char* label, int* currentItem, const char* const items[], int itemsCount, float labelWidth = 100.0f);
	bool LeftLabelBeginCombo(const char* label, const char* preview_value, ImGuiComboFlags flags = 0, float labelWidth = 100.0f);
	void LeftLabelEndCombo();


	bool LeftLabelModelAsset(const char* label, AssetHandle<Rendering::Model>* modelRef);
	bool LeftLabelTextureAsset(const char* label, AssetHandle<Texture>* modelRef);
	bool LeftLabelMaterialAsset(const char* label, AssetHandle<Material>* materialRef);

	bool ComponentHeader(const char* name, bool* removeRequested);
} // namespace Engine

#endif // CPP_ENGINE_INSPECTORUI_H
