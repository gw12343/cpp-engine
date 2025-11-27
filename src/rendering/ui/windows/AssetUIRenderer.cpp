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
#include <algorithm>
#include <cstring>

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
		m_currentDirectory = "resources";
		ScanDirectory(m_currentDirectory);
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

		// Two-column layout: directory tree on left, file grid on right
		ImGui::Columns(2, "browser_columns", true);
		
		// Set initial column width on first frame
		static bool firstTime = true;
		if (firstTime) {
			ImGui::SetColumnWidth(0, 200.0f);
			firstTime = false;
		}

		// Left panel: Directory tree
		ImGui::BeginChild("DirectoryTree", ImVec2(0, 0), true);
		RenderDirectoryTree();
		ImGui::EndChild();

		ImGui::NextColumn();

		// Right panel: File grid
		ImGui::BeginChild("FileGrid", ImVec2(0, 0), true);
		RenderFileGrid();
		ImGui::EndChild();

		ImGui::Columns(1);

		ImGui::End();
	}

	void AssetUIRenderer::RenderDirectoryTree()
	{
		// Render root folders
		RenderDirectoryTreeNode("resources", "resources");
		RenderDirectoryTreeNode("scripts", "scripts");
	}

	void AssetUIRenderer::RenderDirectoryTreeNode(const std::string& dirPath, const std::string& dirName)
	{
		if (!fs::exists(dirPath) || !fs::is_directory(dirPath))
			return;

		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
		
		// Highlight if this is the current directory
		if (dirPath == m_currentDirectory) {
			flags |= ImGuiTreeNodeFlags_Selected;
		}

		// Count subdirectories with error handling
		int subdirCount = 0;
		try {
			for (const auto& entry : fs::directory_iterator(dirPath)) {
				if (entry.is_directory()) {
					// Skip hidden directories
					std::string dirname = entry.path().filename().string();
					if (dirname[0] != '.') {
						subdirCount++;
					}
				}
			}
		} catch (const std::exception& e) {
			GetUI().log->warn("Error reading directory {}: {}", dirPath, e.what());
			subdirCount = 0;
		}

		// If no subdirectories, make it a leaf
		if (subdirCount == 0) {
			flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
		}

		// Render tree node with folder icon
		ImGui::PushID(dirPath.c_str());
		bool nodeOpen = ImGui::TreeNodeEx(dirName.c_str(), flags);

		// Handle selection
		if (ImGui::IsItemClicked()) {
			m_currentDirectory = dirPath;
			ScanDirectory(m_currentDirectory);
		}

		// Render children if open and has subdirectories
		if (nodeOpen && subdirCount > 0) {
			std::vector<std::string> subdirs;
			try {
				for (const auto& entry : fs::directory_iterator(dirPath)) {
					if (entry.is_directory()) {
						std::string dirname = entry.path().filename().string();
						// Skip hidden directories
						if (dirname[0] != '.') {
							subdirs.push_back(dirname);
						}
					}
				}
			} catch (const std::exception& e) {
				GetUI().log->warn("Error reading directory {}: {}", dirPath, e.what());
			}
			
			// Sort alphabetically
			std::sort(subdirs.begin(), subdirs.end());

			for (const auto& subdir : subdirs) {
				std::string subdirPath = dirPath + "/" + subdir;
				RenderDirectoryTreeNode(subdirPath, subdir);
			}
			ImGui::TreePop();
		}

		ImGui::PopID();
	}

	void AssetUIRenderer::RenderFileGrid()
	{
		float padding     = 8.0f;
		int   columnCount = static_cast<int>(ImGui::GetContentRegionAvail().x / (iconSize + padding));
		if (columnCount < 1) columnCount = 1;

		ImGui::Columns(columnCount, nullptr, false);

		// Render files and directories
		for (const auto& fileEntry : m_currentFiles) {
			RenderFileCard(fileEntry.path, fileEntry.filename, fileEntry.isDirectory);
			ImGui::NextColumn();
		}

		ImGui::Columns(1);

		// Context menu for empty space (right-click on background)
		if (ImGui::BeginPopupContextWindow("FileGridContext", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)) {
			if (ImGui::MenuItem("Refresh")) {
				RefreshCurrentDirectory();
			}
			ImGui::EndPopup();
		}
	}

	void AssetUIRenderer::RenderFileCard(const std::string& path, const std::string& filename, bool isDirectory)
	{
		ImGui::PushID(path.c_str());

		fs::path fsPath(path);
		std::string ext = fsPath.extension().string();

		// Measure text for card sizing
		float wrapWidth = iconSize;
		ImVec2 textSize = ImGui::CalcTextSize(filename.c_str(), nullptr, false, wrapWidth);

		// Get appropriate icon
		void* iconID = GetIconForFile(path, ext, isDirectory);

		// Card background and interaction
		ImVec2 startPos = ImGui::GetCursorScreenPos();
		ImVec2 itemSize(iconSize, iconSize + textSize.y + 7.0f);

		std::string btnId = "card_" + path;
		bool clicked = ImGui::InvisibleButton(btnId.c_str(), itemSize);

		// Handle double-click on directory to navigate into it
		if (clicked && isDirectory && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
			m_currentDirectory = path;
			ScanDirectory(m_currentDirectory);
		}

		// Handle right-click for context menu
		if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
			m_rightClickedFile = path;
			ImGui::OpenPopup("FileContextMenu");
		}

		// Drag-drop source for asset files
	if (!isDirectory && ImGui::BeginDragDropSource()) {
		struct PayloadData {
			const char* type;
			char        id[64];  // Must match size expected by InspectorUI
		};
		PayloadData payload;

		// Determine payload type based on extension
		const char* payloadType = "ASSET_FILE";
		const char* assetType = "Unknown";

		if (ext == ".png") {
			payloadType = "ASSET_TEXTURE";
			assetType = "Texture";
			
			// Load texture and get GUID
			auto handle = GetAssetManager().Load<Texture>(path);
			strncpy(payload.id, handle.GetID().c_str(), sizeof(payload.id));
		} else if (ext == ".obj") {
			payloadType = "ASSET_MODEL";
			assetType = "Rendering::Model";
			
			auto handle = GetAssetManager().Load<Rendering::Model>(path);
			strncpy(payload.id, handle.GetID().c_str(), sizeof(payload.id));
		} else if (ext == ".material") {
			payloadType = "ASSET_MATERIAL";
			assetType = "Material";
			
			auto handle = GetAssetManager().Load<Material>(path);
			strncpy(payload.id, handle.GetID().c_str(), sizeof(payload.id));
		} else if (ext == ".wav") {
			payloadType = "ASSET_SOUND";
			assetType = "Audio::SoundBuffer";
			
			auto handle = GetAssetManager().Load<Audio::SoundBuffer>(path);
			strncpy(payload.id, handle.GetID().c_str(), sizeof(payload.id));
		} else if (ext == ".anim") {
			payloadType = "ASSET_ANIMATION";
			assetType = "Animation";
			
			auto handle = GetAssetManager().Load<Animation>(path);
			strncpy(payload.id, handle.GetID().c_str(), sizeof(payload.id));
		}

		payload.type = assetType;
		payload.id[sizeof(payload.id) - 1] = '\0';

		ImGui::SetDragDropPayload(payloadType, &payload, sizeof(payload));
		ImGui::Text("%s: %s", assetType, filename.c_str());

		ImGui::EndDragDropSource();
	}	

		// Selection/hover effect
		bool isSelected = (m_selectedFile == path);
		if (ImGui::IsItemHovered() || isSelected) {
			ImU32 col = isSelected ? ImGui::GetColorU32(ImGuiCol_ButtonActive) : ImGui::GetColorU32(ImGuiCol_ButtonHovered);
			ImGui::GetWindowDrawList()->AddRectFilled(startPos, ImVec2(startPos.x + itemSize.x, startPos.y + itemSize.y), col, 6.0f);
		}

		if (clicked && !isDirectory) {
			m_selectedFile = path;
			
			// If it's a material file, select it in the material editor
			if (ext == ".material") {
				auto handle = GetAssetManager().Load<Material>(path);
				GetUI().m_selectedMaterial = handle;
			}
		}

		// Draw icon
		ImGui::SetCursorScreenPos(startPos);
		// Flip UV coordinates only for framebuffer-rendered previews (models and materials)
		// Regular textures are already right-side up
		if (ext == ".obj" || ext == ".material") {
			// Framebuffer textures need Y-flip: (0,1) to (1,0)
			ImGui::Image(iconID, ImVec2(iconSize, iconSize), ImVec2(0, 1), ImVec2(1, 0));
		} else {
			// Regular textures and icons: default (0,0) to (1,1)
			ImGui::Image(iconID, ImVec2(iconSize, iconSize));
		}

		// Draw filename
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f);
		ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + iconSize);
		ImGui::TextWrapped("%s", filename.c_str());
		ImGui::PopTextWrapPos();

		// Context menu popup
		if (ImGui::BeginPopup("FileContextMenu")) {
			if (m_rightClickedFile == path) {
				ImGui::Text("%s", filename.c_str());
				ImGui::Separator();
				
				if (ImGui::MenuItem("Delete")) {
					DeleteFile(path);
					ImGui::CloseCurrentPopup();
				}
				
				if (!isDirectory && ImGui::MenuItem("Duplicate")) {
					DuplicateFile(path);
					ImGui::CloseCurrentPopup();
				}
				
				if (ImGui::MenuItem("Rename")) {
					m_renamingFile = true;
					strncpy(m_renameBuffer, filename.c_str(), sizeof(m_renameBuffer));
					m_renameBuffer[sizeof(m_renameBuffer) - 1] = '\0';
					ImGui::CloseCurrentPopup();
				}
			}
			ImGui::EndPopup();
		}

		// Rename modal
		if (m_renamingFile && m_rightClickedFile == path) {
			ImGui::OpenPopup("Rename File");
		}

		if (ImGui::BeginPopupModal("Rename File", &m_renamingFile, ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::Text("Enter new name:");
			ImGui::InputText("##rename", m_renameBuffer, sizeof(m_renameBuffer));
			
			if (ImGui::Button("OK") || ImGui::IsKeyPressed(ImGuiKey_Enter)) {
				fs::path parentPath = fs::path(path).parent_path();
				std::string newPath = parentPath.string() + "/" + std::string(m_renameBuffer);
				RenameFile(path, newPath);
				m_renamingFile = false;
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel") || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
				m_renamingFile = false;
				ImGui::CloseCurrentPopup();
			}
			
			ImGui::EndPopup();
		}

		ImGui::PopID();
	}

	void* AssetUIRenderer::GetIconForFile(const std::string& path, const std::string& extension, bool isDirectory)
	{
		if (isDirectory) {
			return reinterpret_cast<void*>(static_cast<intptr_t>(GetUI().m_folderIconTexture->GetID()));
		}

		// Return appropriate icon based on file extension
		if (extension == ".png" || extension == ".jpg" || extension == ".jpeg") {
			// For textures, show the actual texture
			try {
				auto handle = GetAssetManager().Load<Texture>(path);
				auto* tex = GetAssetManager().Get(handle);
				if (tex) {
					return reinterpret_cast<void*>(static_cast<intptr_t>(tex->GetID()));
				}
			} catch (const std::exception& e) {
				GetUI().log->warn("Failed to load texture for preview {}: {}", path, e.what());
			}
			return reinterpret_cast<void*>(static_cast<intptr_t>(GetUI().m_fileIconTexture->GetID()));
		}
		else if (extension == ".obj") {
			// For models, render preview with error handling
			// Check if we've already tried to load this model
			if (m_loadedModelPaths.find(path) == m_loadedModelPaths.end()) {
				// First time seeing this model, try to load it
				m_loadedModelPaths.insert(path);
				try {
					GetUI().log->debug("Loading model for preview: {}", path);
					auto handle = GetAssetManager().Load<Rendering::Model>(path);
					auto* model = GetAssetManager().Get(handle);
					if (model) {
						auto& preview = m_modelPreviews[handle.GetID()];
						preview.width = static_cast<int>(MODEL_PREVIEW_SIZE);
						preview.height = static_cast<int>(MODEL_PREVIEW_SIZE);
						preview.Render(model, GetRenderer().GetModelPreviewShader());
					}
				} catch (const std::exception& e) {
					GetUI().log->warn("Failed to load model for preview {}: {}", path, e.what());
				}
			}
			
			// Try to get the preview from cache
			auto handle = GetAssetManager().GetHandleFromPath<Rendering::Model>(path);
			if (handle.IsValid()) {
				auto it = m_modelPreviews.find(handle.GetID());
				if (it != m_modelPreviews.end()) {
					return reinterpret_cast<void*>(static_cast<intptr_t>(it->second.texture));
				}
			}
			return reinterpret_cast<void*>(static_cast<intptr_t>(GetUI().m_modelIconTexture->GetID()));
		}
		else if (extension == ".material") {
			// For materials, render preview on sphere with error handling
			// Check if we've already tried to load this material
			if (m_loadedMaterialPaths.find(path) == m_loadedMaterialPaths.end()) {
				// First time seeing this material, try to load it
				m_loadedMaterialPaths.insert(path);
				try {
					auto handle = GetAssetManager().Load<Material>(path);
					auto* mat = GetAssetManager().Get(handle);
					if (mat) {
						auto& preview = m_materialPreviews[handle.GetID()];
						preview.width = static_cast<int>(MATERIAL_PREVIEW_SIZE);
						preview.height = static_cast<int>(MATERIAL_PREVIEW_SIZE);
						preview.Render(mat, GetRenderer().GetMaterialPreviewShader());
					}
				} catch (const std::exception& e) {
					GetUI().log->warn("Failed to load material for preview {}: {}", path, e.what());
				}
			}
			
			// Try to get the preview from cache
			auto handle = GetAssetManager().GetHandleFromPath<Material>(path);
			if (handle.IsValid()) {
				auto it = m_materialPreviews.find(handle.GetID());
				if (it != m_materialPreviews.end()) {
					return reinterpret_cast<void*>(static_cast<intptr_t>(it->second.texture));
				}
			}
			return reinterpret_cast<void*>(static_cast<intptr_t>(GetUI().m_materialIconTexture->GetID()));
		}
		else if (extension == ".wav" || extension == ".mp3" || extension == ".ogg") {
			return reinterpret_cast<void*>(static_cast<intptr_t>(GetUI().m_audioIconTexture->GetID()));
		}
		else if (extension == ".anim") {
			return reinterpret_cast<void*>(static_cast<intptr_t>(GetUI().m_animationIconTexture->GetID()));
		}
		else if (extension == ".bin") {
			return reinterpret_cast<void*>(static_cast<intptr_t>(GetUI().m_terrainIconTexture->GetID()));
		}
		else if (extension == ".glsl" || extension == ".vert" || extension == ".frag" || extension == ".comp") {
			return reinterpret_cast<void*>(static_cast<intptr_t>(GetUI().m_shaderIconTexture->GetID()));
		}
		else if (extension == ".efk") {
			return reinterpret_cast<void*>(static_cast<intptr_t>(GetUI().m_particleIconTexture->GetID()));
		}

		// Generic file icon for unknown types
		return reinterpret_cast<void*>(static_cast<intptr_t>(GetUI().m_fileIconTexture->GetID()));
	}

	void AssetUIRenderer::ScanDirectory(const std::string& dirPath)
	{
		m_currentFiles.clear();

		if (!fs::exists(dirPath) || !fs::is_directory(dirPath))
			return;

		std::vector<FileEntry> directories;
		std::vector<FileEntry> files;

		for (const auto& entry : fs::directory_iterator(dirPath)) {
			FileEntry fileEntry;
			fileEntry.path = entry.path().string();
			fileEntry.filename = entry.path().filename().string();
			fileEntry.isDirectory = entry.is_directory();

			// Skip hidden files and meta files
			if (fileEntry.filename[0] == '.' || fileEntry.filename.find(".meta") != std::string::npos) {
				continue;
			}

			if (!fileEntry.isDirectory) {
				fileEntry.extension = entry.path().extension().string();
				fileEntry.size = entry.file_size();
				
				// Check if it's a known asset type
				fileEntry.isAsset = (fileEntry.extension == ".png" || fileEntry.extension == ".jpg" ||
				                     fileEntry.extension == ".obj" || fileEntry.extension == ".material" ||
				                     fileEntry.extension == ".wav" || fileEntry.extension == ".anim" ||
				                     fileEntry.extension == ".bin" || fileEntry.extension == ".efk");
				
				files.push_back(fileEntry);
			} else {
				directories.push_back(fileEntry);
			}
		}

		// Sort alphabetically
		std::sort(directories.begin(), directories.end(), [](const FileEntry& a, const FileEntry& b) {
			return a.filename < b.filename;
		});
		std::sort(files.begin(), files.end(), [](const FileEntry& a, const FileEntry& b) {
			return a.filename < b.filename;
		});

		// Add directories first, then files
		m_currentFiles.insert(m_currentFiles.end(), directories.begin(), directories.end());
		m_currentFiles.insert(m_currentFiles.end(), files.begin(), files.end());
	}

	void AssetUIRenderer::RefreshCurrentDirectory()
	{
		ScanDirectory(m_currentDirectory);
	}

	void AssetUIRenderer::DeleteFile(const std::string& path)
	{
		if (!fs::exists(path)) {
			GetUI().log->warn("Cannot delete, file does not exist: {}", path);
			return;
		}

		std::error_code ec;
		fs::path fsPath(path);
		std::string metaPath = path + ".meta";

		// Unload from asset manager if it's an asset
		std::string ext = fsPath.extension().string();
		
		// Try to unload various asset types
		if (ext == ".png") {
			auto handle = GetAssetManager().GetHandleFromPath<Texture>(path);
			if (handle.IsValid()) GetAssetManager().Unload(handle);
		} else if (ext == ".obj") {
			auto handle = GetAssetManager().GetHandleFromPath<Rendering::Model>(path);
			if (handle.IsValid()) GetAssetManager().Unload(handle);
		} else if (ext == ".material") {
			auto handle = GetAssetManager().GetHandleFromPath<Material>(path);
			if (handle.IsValid()) GetAssetManager().Unload(handle);
		} else if (ext == ".wav") {
			auto handle = GetAssetManager().GetHandleFromPath<Audio::SoundBuffer>(path);
			if (handle.IsValid()) GetAssetManager().Unload(handle);
		} else if (ext == ".anim") {
			auto handle = GetAssetManager().GetHandleFromPath<Animation>(path);
			if (handle.IsValid()) GetAssetManager().Unload(handle);
		}

		// Delete the file
		if (fs::is_directory(path)) {
			fs::remove_all(path, ec);
		} else {
			fs::remove(path, ec);
		}

		if (ec) {
			GetUI().log->error("Failed to delete file: {} - {}", path, ec.message());
			return;
		}

		// Delete meta file if it exists
		if (fs::exists(metaPath)) {
			fs::remove(metaPath, ec);
		}

		GetUI().log->info("Deleted: {}", path);
		RefreshCurrentDirectory();
	}

	void AssetUIRenderer::DuplicateFile(const std::string& path)
	{
		if (!fs::exists(path) || fs::is_directory(path)) {
			GetUI().log->warn("Cannot duplicate, file does not exist or is a directory: {}", path);
			return;
		}

		fs::path fsPath(path);
		std::string stem = fsPath.stem().string();
		std::string ext = fsPath.extension().string();
		std::string parentPath = fsPath.parent_path().string();

		// Find unique name
		std::string newPath;
		int copyNum = 1;
		do {
			std::string newName = stem + "_copy";
			if (copyNum > 1) {
				newName += std::to_string(copyNum);
			}
			newPath = parentPath + "/" + newName + ext;
			copyNum++;
		} while (fs::exists(newPath));

		// Copy file
		std::error_code ec;
		fs::copy_file(path, newPath, ec);

		if (ec) {
			GetUI().log->error("Failed to duplicate file: {} - {}", path, ec.message());
			return;
		}

		// Create new .meta file with new GUID
		std::string newMetaPath = newPath + ".meta";
		GetAssetManager().EnsureMetaFile<Texture>(newPath); // Use any type, just need to generate GUID

		GetUI().log->info("Duplicated: {} -> {}", path, newPath);
		RefreshCurrentDirectory();
	}

	void AssetUIRenderer::RenameFile(const std::string& oldPath, const std::string& newPath)
	{
		if (!fs::exists(oldPath)) {
			GetUI().log->warn("Cannot rename, file does not exist: {}", oldPath);
			return;
		}

		if (fs::exists(newPath)) {
			GetUI().log->warn("Cannot rename, target already exists: {}", newPath);
			return;
		}

		std::error_code ec;
		
		// Use AssetManager's rename for supported assets
		fs::path fsPath(oldPath);
		std::string ext = fsPath.extension().string();
		
		// Only Texture and Model types support RenameAsset (have m_name field)
		if (ext == ".png") {
			GetAssetManager().RenameAsset<Texture>(oldPath, newPath);
		} else if (ext == ".obj") {
			GetAssetManager().RenameAsset<Rendering::Model>(oldPath, newPath);
		} else {
			// For other file types, use filesystem rename
			fs::rename(oldPath, newPath, ec);
			if (ec) {
				GetUI().log->error("Failed to rename file: {} - {}", oldPath, ec.message());
				return;
			}
			
			// Also rename .meta file if it exists
			std::string oldMetaPath = oldPath + ".meta";
			std::string newMetaPath = newPath + ".meta";
			if (fs::exists(oldMetaPath)) {
				fs::rename(oldMetaPath, newMetaPath, ec);
			}
		}

		GetUI().log->info("Renamed: {} -> {}", oldPath, newPath);
		RefreshCurrentDirectory();
	}

	void AssetUIRenderer::RenderContextMenu()
	{
		// This is now handled inline in RenderFileCard
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