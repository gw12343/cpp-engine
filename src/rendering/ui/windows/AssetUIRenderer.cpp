//
// Created by gabe on 8/17/25.
//

#include "AssetUIRenderer.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "functional"
#include "terrain/TerrainTile.h"
#include "rendering/Renderer.h"
#include "efsw/efsw.hpp"
#include "utils/Utils.h"
#include "rendering/particles/Particle.h"
#include "animation/Animation.h"
#include <filesystem>
#include <functional>

#define DEFAULT_ICON_SIZE 128.0f
namespace fs = std::filesystem;

namespace Engine {

	float iconSize = DEFAULT_ICON_SIZE;

#define DELETE_IF(name, type, extt, fp)                                                                                                                                                                                                        \
	void DeleteAssetIf_##name(const std::string& filePath, std::string metaPath, const std::string& ext, const std::string& dir)                                                                                                               \
	{                                                                                                                                                                                                                                          \
		if (dir == fp && ext == extt) {                                                                                                                                                                                                        \
			AssetHandle<type> handle = GetAssetManager().Load<type>(filePath);                                                                                                                                                                 \
			GetAssetManager().Unload(handle);                                                                                                                                                                                                  \
                                                                                                                                                                                                                                               \
			if (fs::exists(metaPath)) {                                                                                                                                                                                                        \
				std::error_code ec;                                                                                                                                                                                                            \
				GetUI().log->info("path: {}", metaPath);                                                                                                                                                                                       \
				std::filesystem::remove(metaPath, ec);                                                                                                                                                                                         \
                                                                                                                                                                                                                                               \
				GetUI().log->info("deleted {} metafile: {}", #name, metaPath);                                                                                                                                                                 \
			}                                                                                                                                                                                                                                  \
		}                                                                                                                                                                                                                                      \
	}

	DELETE_IF(Material, Material, ".material", "resources/materials/")
	DELETE_IF(Model, Rendering::Model, ".obj", "resources/models/")
	DELETE_IF(Particle, Particle, ".efk", "resources/particles/")
	DELETE_IF(Sound, Audio::SoundBuffer, ".wav", "resources/sounds/")
	DELETE_IF(Terrain, Terrain::TerrainTile, ".bin", "resources/terrain/")
	DELETE_IF(Texture, Texture, ".png", "resources/textures/")
	DELETE_IF(Animation, Animation, ".anim", "resources/animations/")


	void AssetWatcher::handleFileAction(efsw::WatchID watchid, const std::string& dir, const std::string& filename, efsw::Action action, std::string oldFilename)
	{
		fs::path    filePath = dir + filename;
		fs::path    metaPath = dir + filename + ".meta";
		std::string ext      = filePath.extension();


		// TODO add loading
		switch (action) {
			case efsw::Actions::Delete:
				GetUI().log->debug("Detected deleted file: {}", filePath.c_str());

				DeleteAssetIf_Material(filePath, metaPath, ext, dir);
				DeleteAssetIf_Model(filePath, metaPath, ext, dir);
				DeleteAssetIf_Particle(filePath, metaPath, ext, dir);
				DeleteAssetIf_Sound(filePath, metaPath, ext, dir);
				DeleteAssetIf_Terrain(filePath, metaPath, ext, dir);
				DeleteAssetIf_Texture(filePath, metaPath, ext, dir);


				break;
		}
	}


	AssetUIRenderer::AssetUIRenderer()
	{
		drawFuncs = {{typeid(Terrain::TerrainTile), [this]() { DrawTerrainAssets(); }},
		             {typeid(Audio::SoundBuffer), [this]() { DrawSoundAssets(); }},
		             {typeid(Rendering::Model), [this]() { DrawModelAssets(); }},
		             {typeid(Material), [this]() { DrawMaterialAssets(); }},
		             {typeid(Animation), [this]() { DrawAnimationAssets(); }},
		             {typeid(Texture), [this]() { DrawTextureAssets(); }}

		};
	}


	void AssetUIRenderer::RenderAssetWindow()
	{
		ImGui::Begin("Assets");

		ImGui::SliderFloat("Icon Size", &iconSize, 16.0f, 256.0f, "%.0f", ImGuiSliderFlags_AlwaysClamp);

		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window && ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup)) {
			if (ImGui::GetIO().KeyCtrl) {
				float scroll = ImGui::GetIO().MouseWheel;
				if (scroll != 0.0f) {
					iconSize += scroll * 8.0f;
					iconSize = std::clamp(iconSize, 16.0f, 256.0f);
				}
			}
		}


		if (ImGui::BeginTabBar("AssetTabs")) {
			for (auto& [type, drawFn] : drawFuncs) {
				const char* label = type.name(); // fallback label
				if (type == typeid(Texture))
					label = "Textures";
				else if (type == typeid(Rendering::Model))
					label = "Models";
				else if (type == typeid(Audio::SoundBuffer))
					label = "Sounds";
				else if (type == typeid(Animation))
					label = "Animations";
				else if (type == typeid(Material))
					label = "Materials";
				else if (type == typeid(Terrain::TerrainTile))
					label = "Terrains";
				// Add custom labels per type

				if (ImGui::BeginTabItem(label)) {
					ImGui::BeginChild("AssetScroll", ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
					drawFn();
					ImGui::EndChild();
					ImGui::EndTabItem();
				}
			}
			ImGui::EndTabBar();
		}

		ImGui::End();
	}

	void AssetUIRenderer::DrawSoundAssets()
	{
		auto& storage = GetAssetManager().GetStorage<Audio::SoundBuffer>();

		float padding     = 8.0f;
		int   columnCount = static_cast<int>(ImGui::GetContentRegionAvail().x / (iconSize + padding));
		if (columnCount < 1) columnCount = 1;

		ImGui::Columns(columnCount, nullptr, false);

		for (auto& [id, sndPtr] : storage.guidToAsset) {
			if (!sndPtr) continue;
			ImGui::PushID(("snd" + id).c_str());


			// --- Measure text to get correct rect size ---
			std::string label     = sndPtr->name;
			float       wrapWidth = iconSize; // restrict text to same width as preview
			ImVec2      textSize  = ImGui::CalcTextSize(label.c_str(), nullptr, false, wrapWidth);
			SelectableBackground(textSize, id, "Audio::SoundBuffer", "ASSET_SOUND");


			ImGui::Image(reinterpret_cast<void*>(static_cast<intptr_t>(GetUI().m_audioIconTexture->GetID())), ImVec2(iconSize, iconSize));
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f);
			ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + iconSize);
			ImGui::TextWrapped("%s", label.c_str());
			ImGui::PopTextWrapPos();

			ImGui::NextColumn();
			ImGui::PopID();
		}

		ImGui::Columns(1);
	}

	void AssetUIRenderer::DrawTerrainAssets()
	{
		auto& storage = GetAssetManager().GetStorage<Terrain::TerrainTile>();

		float padding     = 8.0f;
		int   columnCount = static_cast<int>(ImGui::GetContentRegionAvail().x / (iconSize + padding));
		if (columnCount < 1) columnCount = 1;

		ImGui::Columns(columnCount, nullptr, false);

		for (auto& [id, terrainPtr] : storage.guidToAsset) {
			if (!terrainPtr) continue;
			ImGui::PushID(("tex" + id).c_str());


			ImGui::Image(GetUI().m_terrainIconTexture->GetID(), ImVec2(iconSize, iconSize));
			ImGui::TextWrapped("Terrain %s", terrainPtr->name.c_str());

			ImGui::NextColumn();
			ImGui::PopID();
		}

		ImGui::Columns(1);
	}

	void AssetUIRenderer::DrawAnimationAssets()
	{
		auto& storage = GetAssetManager().GetStorage<Animation>();

		float padding     = 8.0f;
		int   columnCount = static_cast<int>(ImGui::GetContentRegionAvail().x / (iconSize + padding));
		if (columnCount < 1) columnCount = 1;

		ImGui::Columns(columnCount, nullptr, false);

		for (auto& [id, animPtr] : storage.guidToAsset) {
			if (!animPtr) continue;
			ImGui::PushID(("snd" + id).c_str());


			// --- Measure text to get correct rect size ---
			std::string label     = animPtr->name;
			float       wrapWidth = iconSize; // restrict text to same width as preview
			ImVec2      textSize  = ImGui::CalcTextSize(label.c_str(), nullptr, false, wrapWidth);
			SelectableBackground(textSize, id, "Animation", "ASSET_ANIMATION");


			ImGui::Image(reinterpret_cast<void*>(static_cast<intptr_t>(GetUI().m_animationIconTexture->GetID())), ImVec2(iconSize, iconSize));
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f);
			ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + iconSize);
			ImGui::TextWrapped("%s", label.c_str());
			ImGui::PopTextWrapPos();

			ImGui::NextColumn();
			ImGui::PopID();
		}

		ImGui::Columns(1);
	}

	void AssetUIRenderer::DrawTextureAssets()
	{
		auto& storage = GetAssetManager().GetStorage<Texture>();

		float padding     = 8.0f;
		int   columnCount = static_cast<int>(ImGui::GetContentRegionAvail().x / (iconSize + padding));
		if (columnCount < 1) columnCount = 1;

		ImGui::Columns(columnCount, nullptr, false);

		for (auto& [id, texPtr] : storage.guidToAsset) {
			if (!texPtr) continue;
			ImGui::PushID(("tex" + id).c_str());


			// --- Measure text to get correct rect size ---
			std::string label     = texPtr->GetName();
			float       wrapWidth = iconSize; // restrict text to same width as preview
			ImVec2      textSize  = ImGui::CalcTextSize(label.c_str(), nullptr, false, wrapWidth);
			SelectableBackground(textSize, id, "Texture", "ASSET_TEXTURE");


			ImGui::Image(reinterpret_cast<void*>(static_cast<intptr_t>(texPtr->GetID())), ImVec2(iconSize, iconSize));
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f);
			ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + iconSize);
			ImGui::TextWrapped("%s", label.c_str());
			ImGui::PopTextWrapPos();

			ImGui::NextColumn();
			ImGui::PopID();
		}

		ImGui::Columns(1);
	}

	void AssetUIRenderer::DrawMaterialAssets()
	{
		auto& storage = GetAssetManager().GetStorage<Material>();

		float padding     = 8.0f;
		int   columnCount = static_cast<int>(ImGui::GetContentRegionAvail().x / (iconSize + padding));
		if (columnCount < 1) columnCount = 1;

		// Start of region
		ImGui::PushID("MaterialRegion");
		ImGui::BeginChild("MaterialRegionChild", ImGui::GetContentRegionAvail(), false, ImGuiWindowFlags_NoScrollbar);

		ImGui::Columns(columnCount, nullptr, false);

		for (auto& [id, matPtr] : storage.guidToAsset) {
			if (!matPtr) continue;
			ImGui::PushID(("mat" + id).c_str());
			auto tx = GetAssetManager().Get(matPtr->GetDiffuseTexture());

			// --- Measure text to get correct rect size ---
			std::string label     = matPtr->GetName();
			float       wrapWidth = iconSize; // restrict text to same width as preview
			ImVec2      textSize  = ImGui::CalcTextSize(label.c_str(), nullptr, false, wrapWidth);

			if (SelectableBackground(textSize, id, "Material", "ASSET_MATERIAL")) {
				GetUI().m_selectedMaterial = AssetHandle<Material>(id);
			}

			if (tx != NULL) ImGui::Image(reinterpret_cast<void*>(static_cast<intptr_t>(tx->GetID())), ImVec2(iconSize, iconSize), ImVec2(0, 1), ImVec2(1, 0));

			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f);
			ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + iconSize);
			ImGui::TextWrapped("%s", label.c_str());
			ImGui::PopTextWrapPos();

			ImGui::NextColumn();
			ImGui::PopID();
		}

		ImGui::Columns(1);

		// Right-click context menu for the region
		if (ImGui::BeginPopupContextWindow("MaterialRegionContext", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)) {
			if (ImGui::MenuItem("Create New Material +")) {
				// Just print something
				printf("Create New Material clicked!\n");
			}
			ImGui::EndPopup();
		}

		ImGui::EndChild();
		ImGui::PopID();
	}


	void AssetUIRenderer::DrawModelAssets()
	{
		auto& storage = GetAssetManager().GetStorage<Rendering::Model>();

		float padding     = 8.0f;
		int   columnCount = static_cast<int>(ImGui::GetContentRegionAvail().x / (iconSize + padding));
		if (columnCount < 1) columnCount = 1;

		ImGui::Columns(columnCount, nullptr, false);

		for (auto& [id, model] : storage.guidToAsset) {
			if (!model) continue;
			ImGui::PushID(("model" + id).c_str());

			auto& preview  = m_modelPreviews[id];
			preview.width  = static_cast<int>(MODEL_PREVIEW_SIZE);
			preview.height = static_cast<int>(MODEL_PREVIEW_SIZE);
			preview.Render(model.get(), GetRenderer().GetModelPreviewShader());

			// --- Measure text to get correct rect size ---
			std::string label     = model->m_name;
			float       wrapWidth = iconSize; // restrict text to same width as preview
			ImVec2      textSize  = ImGui::CalcTextSize(label.c_str(), nullptr, false, wrapWidth);

			SelectableBackground(textSize, id, "Rendering::Model", "ASSET_MODEL");

			ImGui::Image(reinterpret_cast<void*>(static_cast<intptr_t>(preview.texture)), ImVec2(iconSize, iconSize), ImVec2(0, 1), ImVec2(1, 0));

			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f);
			ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + iconSize);
			ImGui::TextWrapped("%s", label.c_str());
			ImGui::PopTextWrapPos();

			ImGui::NextColumn();
			ImGui::PopID();
		}


		ImGui::Columns(1);
	}

	bool AssetUIRenderer::SelectableBackground(ImVec2 textSize, std::string id, const char* type, const char* typeName)
	{
		ImVec2 startPos = ImGui::GetCursorScreenPos();

		ImVec2 itemSize(iconSize, iconSize + textSize.y + 7.0f);
		bool   clicked = false;
		// Unique ID for button: use asset id
		std::string btnId = "drag_area_" + id;
		if (ImGui::InvisibleButton(btnId.c_str(), itemSize)) {
			clicked = true;
		}
		// Drag-drop source must come immediately after button
		if (ImGui::BeginDragDropSource()) {
			struct PayloadData {
				const char* type;
				char        id[64];
			};
			PayloadData payload;
			payload.type = type;
			strncpy(payload.id, id.c_str(), sizeof(payload.id));
			payload.id[sizeof(payload.id) - 1] = '\0';

			ImGui::SetDragDropPayload(typeName, &payload, sizeof(payload));
			ImGui::Text("Model: %s", id.c_str());

			ImGui::EndDragDropSource();
		}

		// Hover highlight
		if (ImGui::IsItemHovered()) {
			ImU32 col = ImGui::GetColorU32(ImGuiCol_ButtonHovered);
			ImGui::GetWindowDrawList()->AddRectFilled(startPos, ImVec2(startPos.x + itemSize.x, startPos.y + itemSize.y), col, 4.0f);
		}

		// --- Draw content inside rect ---
		ImGui::SetCursorScreenPos(startPos);
		return clicked;
	}
} // namespace Engine

#include "assets/AssetManager.inl"