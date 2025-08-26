//
// Created by gabe on 8/25/25.
//

#ifndef CPP_ENGINE_INSPECTORUI_H
#define CPP_ENGINE_INSPECTORUI_H

#include "imgui.h"
#include "assets/AssetHandle.h"
#include "core/EntityHandle.h"
#include <string>

namespace Engine {

	class Texture;
	class Material;
	class Scene;
	class Particle;
	namespace Terrain {
		class TerrainTile;
	}
	namespace Rendering {
		class Model;
	}

	namespace Audio {
		class SoundBuffer;
	}


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


#define LL_ASSET(name, type) bool LeftLabelAsset##name(const char* label, AssetHandle<type>* assetRef)


	LL_ASSET(Texture, Texture);
	LL_ASSET(Model, Rendering::Model);
	LL_ASSET(Terrain, Terrain::TerrainTile);
	LL_ASSET(Sound, Audio::SoundBuffer);
	LL_ASSET(Scene, Scene);
	LL_ASSET(Particle, Particle);
	LL_ASSET(Material, Material);

	bool LeftLabelEntity(const char* label, EntityHandle* assetRef);


	bool ComponentHeader(const char* name, bool* removeRequested);
} // namespace Engine

#endif // CPP_ENGINE_INSPECTORUI_H
