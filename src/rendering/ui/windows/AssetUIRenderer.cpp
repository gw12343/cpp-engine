//
// Created by gabe on 8/17/25.
//

#include "AssetUIRenderer.h"

#include "imgui_internal.h"
#include "functional"
#include "terrain/TerrainTile.h"
#include "rendering/Renderer.h"
#include "efsw/efsw.hpp"

#include "rendering/particles/Particle.h"
#include "animation/Animation.h"
#include "animation/Skeleton.h"
#include "rendering/ui/UIManager.h"
#include "rendering/ui/EditorSession.h"
#include "assets/Prefab.h"
#include "core/Entity.h"
#include "rendering/ui/IconsFontAwesome6.h"


#include <functional>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <nlohmann/json.hpp>
#include <cstdlib>

#define DEFAULT_ICON_SIZE 128.0f
namespace fs = std::filesystem;

namespace Engine {

	float iconSize = DEFAULT_ICON_SIZE;

	static void OpenInSystemFileViewer(const std::string& path, bool isDirectory)
	{
#ifdef _WIN32
		std::error_code ec;
		fs::path abs = fs::absolute(path, ec);
		if (ec) {
			abs = fs::path(path);
		}
		abs = abs.lexically_normal().make_preferred();
		const std::string p = abs.string();
		const std::string cmd = isDirectory ? ("explorer \"" + p + "\"") : ("explorer /select,\"" + p + "\"");
		std::system(cmd.c_str());
#endif
	}

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

	DELETE_IF(Material, Material, ".material", "assets/materials/")
	DELETE_IF(Model, Rendering::Model, ".obj", "assets/models/")
	DELETE_IF(Particle, Particle, ".efk", "assets/particles/")
	DELETE_IF(Sound, Audio::SoundBuffer, ".wav", "assets/sounds/")
	DELETE_IF(Terrain, Terrain::TerrainTile, ".bin", "assets/terrain/")
	DELETE_IF(Texture, Texture, ".png", "assets/textures/")
	DELETE_IF(Animation, Animation, ".anim", "assets/animations/")


	void AssetWatcher::handleFileAction(efsw::WatchID watchid, const std::string& dir,
								   const std::string& filename, efsw::Action action,
								   const std::string& oldFilename)
	{
		fs::path    filePath = dir + filename;
		fs::path    metaPath = dir + filename + ".meta";
		std::string ext      = filePath.extension().string();


		if (owner) {
			owner->RequestRefresh();
			if (action == efsw::Actions::Modified || action == efsw::Actions::Moved ||
			    action == efsw::Actions::Delete || action == efsw::Actions::Add) {
				owner->QueuePreviewInvalidation(filePath.string());
				if (action == efsw::Actions::Moved && !oldFilename.empty()) {
					owner->QueuePreviewInvalidation((fs::path(dir) / oldFilename).string());
				}
			}
		}
		switch (action) {
			case efsw::Actions::Add:
			case efsw::Actions::Modified:
			case efsw::Actions::Moved:
				break;
			case efsw::Actions::Delete:
				GetUI().log->debug("Detected deleted file: {}", filePath.string());

				DeleteAssetIf_Material(filePath.string(), metaPath.string(), ext, dir);
				DeleteAssetIf_Model(filePath.string(), metaPath.string(), ext, dir);
				DeleteAssetIf_Particle(filePath.string(), metaPath.string(), ext, dir);
				DeleteAssetIf_Sound(filePath.string(), metaPath.string(), ext, dir);
				DeleteAssetIf_Terrain(filePath.string(), metaPath.string(), ext, dir);
				DeleteAssetIf_Texture(filePath.string(), metaPath.string(), ext, dir);


				break;
		}
	}


	AssetUIRenderer::AssetUIRenderer()
	{
		listener.owner     = this;
		m_currentDirectory = "resources";
		ScanDirectory(m_currentDirectory);
	}


	void AssetUIRenderer::NavigateTo(const std::string& path)
	{
		m_currentDirectory = path;
		ScanDirectory(m_currentDirectory);
	}

	void AssetUIRenderer::QueueNavigate(std::string path)
	{
		m_queuedNavigate = std::move(path);
	}

	void AssetUIRenderer::QueueRefresh()
	{
		m_queuedRefresh = true;
	}

	static std::string PreviewCacheKey(std::string p)
	{
		for (char& c : p) {
			if (c == '\\') c = '/';
		}
		constexpr const char* meta = ".meta";
		if (p.size() > 5 && p.compare(p.size() - 5, 5, meta) == 0) {
			p.resize(p.size() - 5);
		}
		return p;
	}

	void AssetUIRenderer::QueuePreviewInvalidation(std::string path)
	{
		std::lock_guard<std::mutex> lock(m_previewMutex);
		m_pendingPreviewInvalidations.push_back(std::move(path));
	}

	void AssetUIRenderer::InvalidatePreview(const std::string& path)
	{
		const std::string key = PreviewCacheKey(path);
		const auto slash = key.find_last_of('/');
		const std::string file = (slash == std::string::npos) ? key : key.substr(slash + 1);
		if (file.empty()) {
			return;
		}

		auto matches = [&](const std::string& k) {
			const std::string n = PreviewCacheKey(k);
			if (n == key) {
				return true;
			}
			const auto s = n.find_last_of('/');
			const std::string name = (s == std::string::npos) ? n : n.substr(s + 1);
			return name == file;
		};

		auto eraseSet = [&](std::unordered_set<std::string>& set) {
			for (auto it = set.begin(); it != set.end();) {
				if (matches(*it)) {
					it = set.erase(it);
				} else {
					++it;
				}
			}
		};
		auto eraseTex = [&]() {
			for (auto it = m_texturePreviewIds.begin(); it != m_texturePreviewIds.end();) {
				if (matches(it->first)) {
					it = m_texturePreviewIds.erase(it);
				} else {
					++it;
				}
			}
		};

		eraseSet(m_loadedModelPaths);
		eraseSet(m_loadedMaterialPaths);
		eraseSet(m_failedPreviewPaths);
		eraseTex();

		auto dropGuid = [&](const std::string& p) {
			auto model = GetAssetManager().GetHandleFromPath<Rendering::Model>(p);
			if (model.IsValid()) {
				m_modelPreviews.erase(model.GetID());
			}
			auto mat = GetAssetManager().GetHandleFromPath<Material>(p);
			if (mat.IsValid()) {
				m_materialPreviews.erase(mat.GetID());
			}
		};
		dropGuid(path);
		dropGuid(key);
		std::string win = key;
		for (char& c : win) {
			if (c == '/') c = '\\';
		}
		dropGuid(win);
	}

	void AssetUIRenderer::FlushPreviewInvalidations()
	{
		std::vector<std::string> pending;
		{
			std::lock_guard<std::mutex> lock(m_previewMutex);
			pending.swap(m_pendingPreviewInvalidations);
		}
		for (const auto& p : pending) {
			InvalidatePreview(p);
		}
	}

	void AssetUIRenderer::ApplyQueuedBrowserOps()
	{
		if (!m_queuedNavigate.empty()) {
			const std::string dest = std::move(m_queuedNavigate);
			m_queuedNavigate.clear();
			m_queuedRefresh = false;
			NavigateTo(dest);
			return;
		}
		if (m_queuedRefresh) {
			m_queuedRefresh = false;
			RefreshCurrentDirectory();
		}
	}

	void AssetUIRenderer::GoUp()
	{
		fs::path p(m_currentDirectory);
		if (p.has_parent_path() && p != p.root_path()) {
			NavigateTo(p.parent_path().string());
		}
	}

	void AssetUIRenderer::RenderAssetWindow()
	{
		ImGui::Begin("Assets");
		m_previewBudget = 4;
		FlushPreviewInvalidations();

		if (m_fsDirty.exchange(false) && !m_renamingFile) {
			RefreshCurrentDirectory();
		}

		if (ImGui::Button("Up")) {
			GoUp();
		}
		ImGui::SameLine();
		ImGui::TextUnformatted(m_currentDirectory.c_str());
		if (ImGui::IsItemClicked()) {
			ImGui::SetClipboardText(m_currentDirectory.c_str());
		}

		ImGui::SetNextItemWidth(-FLT_MIN);
		ImGui::InputTextWithHint("##asset_search", ICON_FA_MAGNIFYING_GLASS " Search this folder...", UI::GetEditor().assetFilter, IM_ARRAYSIZE(UI::GetEditor().assetFilter));
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("Ctrl+Scroll to resize icons");

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

		if (m_confirmDelete) {
			ImGui::OpenPopup("Delete Asset##confirm");
			m_confirmDelete = false;
		}
		if (ImGui::BeginPopupModal("Delete Asset##confirm", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::Text("Delete '%s'?", m_pendingDelete.c_str());
			if (ImGui::Button("Delete", ImVec2(120, 0))) {
				DelFile(m_pendingDelete);
				m_pendingDelete.clear();
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(120, 0))) {
				m_pendingDelete.clear();
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		if (m_openRenamePopup) {
			ImGui::OpenPopup("Rename Asset");
			m_openRenamePopup = false;
		}
		if (ImGui::BeginPopupModal("Rename Asset", &m_renamingFile, ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::Text("Rename:");
			if (ImGui::IsWindowAppearing()) {
				ImGui::SetKeyboardFocusHere();
			}
			ImGui::InputText("##rename", m_renameBuffer, sizeof(m_renameBuffer));

			const bool appearing = ImGui::IsWindowAppearing();
			if ((ImGui::Button("OK") || (!appearing && ImGui::IsKeyPressed(ImGuiKey_Enter))) && m_renameBuffer[0] != '\0') {
				fs::path oldPath(m_rightClickedFile);
				fs::path newName(m_renameBuffer);
				if (!m_renameIsDirectory && newName.extension().empty() && !oldPath.extension().empty()) {
					newName.replace_extension(oldPath.extension());
				}
				const std::string newPath = (oldPath.parent_path() / newName).string();
				RenameFile(m_rightClickedFile, newPath);
				m_renamingFile = false;
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel") || (!appearing && ImGui::IsKeyPressed(ImGuiKey_Escape))) {
				m_renamingFile = false;
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		ApplyQueuedBrowserOps();
		ImGui::End();
	}

	void AssetUIRenderer::RenderDirectoryTree()
	{
		// Render root folders
		RenderDirectoryTreeNode("resources", "resources");
		RenderDirectoryTreeNode("scripts", "scripts");
		RenderDirectoryTreeNode("assets", "assets");
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

		const char* filter = UI::GetEditor().assetFilter;
		for (size_t i = 0; i < m_currentFiles.size(); ++i) {
			FileEntry fileEntry = m_currentFiles[i];
			if (filter[0] && !fileEntry.isDirectory &&
			    !UI::EditorSession::MatchesFilter(fileEntry.filename, filter)) {
				continue;
			}
			RenderFileCard(std::move(fileEntry.path), std::move(fileEntry.filename), fileEntry.isDirectory);
			ImGui::NextColumn();
			if (!m_queuedNavigate.empty()) {
				break;
			}
		}

		ImGui::Columns(1);

		// Context menu for empty space (right-click on background)
		if (ImGui::BeginPopupContextWindow("FileGridContext", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)) {
			if (ImGui::MenuItem("Refresh")) {
				RefreshCurrentDirectory();
			}
			if (ImGui::MenuItem("New Folder")) {
				fs::path dest = fs::path(m_currentDirectory) / "New Folder";
				int n = 1;
				while (fs::exists(dest)) {
					dest = fs::path(m_currentDirectory) / ("New Folder " + std::to_string(n++));
				}
				std::error_code ec;
				fs::create_directory(dest, ec);
				RefreshCurrentDirectory();
			}
			if (ImGui::MenuItem("Open in File Explorer")) {
				OpenInSystemFileViewer(m_currentDirectory, true);
			}
			ImGui::EndPopup();
		}
	}

	void AssetUIRenderer::RenderFileCard(std::string path, std::string filename, bool isDirectory)
	{
		ImGui::PushID(path.c_str());

		fs::path fsPath(path);
		std::string ext = fsPath.extension().string();

		// Measure text for card sizing
		float wrapWidth = iconSize;
		ImVec2 textSize = ImGui::CalcTextSize(filename.c_str(), nullptr, false, wrapWidth);

		// Card background and interaction
		ImVec2 startPos = ImGui::GetCursorScreenPos();
		ImVec2 itemSize(iconSize, iconSize + textSize.y + 7.0f);
		const bool inView = ImGui::IsRectVisible(itemSize);

		void* iconID = GetIconForFile(path, ext, isDirectory, inView);

		std::string btnId = "card_" + path;
		bool clicked = ImGui::InvisibleButton(btnId.c_str(), itemSize);
		const bool cardHovered = ImGui::IsItemHovered();

		// Double-click a folder to enter it. Check hover + double-click (not
		// InvisibleButton's clicked, which fires on release after double-click expired).
		if (isDirectory && cardHovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
			QueueNavigate(path);
		}
		if (!isDirectory && ext == ".prefab" && cardHovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
			auto   handle  = GetAssetManager().Load<Prefab>(path);
			Entity spawned = InstantiatePrefab(handle);
			if (spawned && spawned.IsValid()) {
				GetUI().m_selectedEntity = spawned;
				UI::GetEditor().MarkDirty();
			}
		}

		if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
			m_rightClickedFile = path;
			m_selectedFile     = path;
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

		if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".dds" || ext == ".tga") {
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
		} else if (ext == ".wav" || ext == ".mp3" || ext == ".ogg") {
			payloadType = "ASSET_SOUND";
			assetType = "Audio::SoundBuffer";
			
			auto handle = GetAssetManager().Load<Audio::SoundBuffer>(path);
			strncpy(payload.id, handle.GetID().c_str(), sizeof(payload.id));
		} else if (ext == ".anim") {
			payloadType = "ASSET_ANIMATION";
			assetType = "Animation";
			
			auto handle = GetAssetManager().Load<Animation>(path);
			strncpy(payload.id, handle.GetID().c_str(), sizeof(payload.id));
		} else if (ext == ".ozz") {
			// Shared extension: prefer meta type, else filename heuristic for skeletons.
			bool asSkeleton = false;
			const std::string metaPath = path + ".meta";
			if (fs::exists(metaPath)) {
				try {
					std::ifstream file(metaPath);
					nlohmann::json j;
					file >> j;
					if (j.contains("type")) {
						const std::string type = j["type"].get<std::string>();
						if (type.find("Skeleton") != std::string::npos) asSkeleton = true;
						else if (type.find("Animation") != std::string::npos) asSkeleton = false;
						else {
							// Unknown meta (mesh etc.) — try skeleton name heuristic only.
							const std::string lower = filename;
							asSkeleton = lower.find("skeleton") != std::string::npos || lower.find("skel") != std::string::npos;
						}
					}
				} catch (...) {
					asSkeleton = filename.find("skeleton") != std::string::npos;
				}
			} else {
				asSkeleton = filename.find("skeleton") != std::string::npos || filename.find("skel") != std::string::npos;
			}

			if (asSkeleton) {
				payloadType = "ASSET_SKELETON";
				assetType   = "Skeleton";
				auto handle = GetAssetManager().Load<Skeleton>(path);
				strncpy(payload.id, handle.GetID().c_str(), sizeof(payload.id));
			} else {
				// Animation clip ozz (e.g. idle.ozz) — only if meta says Animation
				payloadType = "ASSET_ANIMATION";
				assetType   = "Animation";
				auto handle = GetAssetManager().Load<Animation>(path);
				strncpy(payload.id, handle.GetID().c_str(), sizeof(payload.id));
			}
		} else if (ext == ".efk") {
            payloadType = "ASSET_PARTICLE";
            assetType = "Particle";

            auto handle = GetAssetManager().Load<Particle>(path);
            strncpy(payload.id, handle.GetID().c_str(), sizeof(payload.id));
        } else if (ext == ".prefab") {
			payloadType = "ASSET_PREFAB";
			assetType   = "Prefab";
			auto handle = GetAssetManager().Load<Prefab>(path);
			strncpy(payload.id, handle.GetID().c_str(), sizeof(payload.id));
		}

		payload.type = assetType;
		payload.id[sizeof(payload.id) - 1] = '\0';

		ImGui::SetDragDropPayload(payloadType, &payload, sizeof(payload));
		ImGui::Text("%s: %s", assetType, filename.c_str());

		ImGui::EndDragDropSource();
	}

		// Must stay immediately after the card button so the popup is tied to that
		// item. Manual OpenPopup-on-press closes when RMB is released.
		if (ImGui::BeginPopupContextItem("FileContextMenu")) {
			m_rightClickedFile = path;
			ImGui::Text("%s", filename.c_str());
			ImGui::Separator();
			if (isDirectory && ImGui::MenuItem("Open")) {
				QueueNavigate(path);
				ImGui::CloseCurrentPopup();
			}
			if (ImGui::MenuItem("Open in File Explorer")) {
				OpenInSystemFileViewer(path, isDirectory);
				ImGui::CloseCurrentPopup();
			}
			if (ImGui::MenuItem("Copy Path")) {
				ImGui::SetClipboardText(path.c_str());
				ImGui::CloseCurrentPopup();
			}
			if (ImGui::MenuItem("Rename")) {
				m_rightClickedFile    = path;
				m_renameIsDirectory   = isDirectory;
				m_renamingFile        = true;
				m_openRenamePopup     = true;
				strncpy(m_renameBuffer, filename.c_str(), sizeof(m_renameBuffer));
				m_renameBuffer[sizeof(m_renameBuffer) - 1] = '\0';
				ImGui::CloseCurrentPopup();
			}
			if (!isDirectory && ImGui::MenuItem("Duplicate")) {
				DuplicateFile(path);
				ImGui::CloseCurrentPopup();
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Delete")) {
				m_pendingDelete = path;
				m_confirmDelete = true;
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		// Selection/hover effect
		bool isSelected = (m_selectedFile == path);
		if (ImGui::IsItemHovered() || isSelected) {
			ImU32 col = isSelected ? ImGui::GetColorU32(ImGuiCol_ButtonActive) : ImGui::GetColorU32(ImGuiCol_ButtonHovered);
			ImGui::GetWindowDrawList()->AddRectFilled(startPos, ImVec2(startPos.x + itemSize.x, startPos.y + itemSize.y), col, 6.0f);
		}

		if (clicked) {
			m_selectedFile = path;
			if (!isDirectory && ext == ".material") {
				auto handle = GetAssetManager().Load<Material>(path);
				GetUI().m_selectedMaterial = handle;
			}
			if (!isDirectory && ext == ".obj") {
				auto handle = GetAssetManager().Load<Rendering::Model>(path);
				GetUI().m_selectedModel = handle;
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

		ImGui::PopID();
	}

	void* AssetUIRenderer::GetIconForFile(const std::string& path, const std::string& extension, bool isDirectory, bool allowLoad)
	{
		if (isDirectory) {
			return reinterpret_cast<void*>(static_cast<intptr_t>(GetUI().m_folderIconTexture->GetID()));
		}

		auto cachedTex = m_texturePreviewIds.find(path);
		if (cachedTex != m_texturePreviewIds.end()) {
			return reinterpret_cast<void*>(static_cast<intptr_t>(cachedTex->second));
		}

		const bool isTexture = (extension == ".png" || extension == ".jpg" || extension == ".jpeg" ||
		                        extension == ".dds" || extension == ".tga");
		if (isTexture) {
			if (m_failedPreviewPaths.count(path) || !allowLoad || m_previewBudget <= 0) {
				return reinterpret_cast<void*>(static_cast<intptr_t>(GetUI().m_fileIconTexture->GetID()));
			}
			--m_previewBudget;
			try {
				auto  handle = GetAssetManager().Load<Texture>(path);
				auto* tex    = GetAssetManager().Get(handle);
				if (tex) {
					m_texturePreviewIds[path] = tex->GetID();
					return reinterpret_cast<void*>(static_cast<intptr_t>(tex->GetID()));
				}
			} catch (const std::exception& e) {
				GetUI().log->warn("Failed to load texture for preview {}: {}", path, e.what());
			}
			m_failedPreviewPaths.insert(path);
			return reinterpret_cast<void*>(static_cast<intptr_t>(GetUI().m_fileIconTexture->GetID()));
		}

		if (extension == ".obj") {
			auto handle = GetAssetManager().GetHandleFromPath<Rendering::Model>(path);
			if (handle.IsValid()) {
				auto it = m_modelPreviews.find(handle.GetID());
				if (it != m_modelPreviews.end() && it->second.texture) {
					return reinterpret_cast<void*>(static_cast<intptr_t>(it->second.texture));
				}
			}
			if (!allowLoad || m_failedPreviewPaths.count(path) || m_previewBudget <= 0) {
				return reinterpret_cast<void*>(static_cast<intptr_t>(GetUI().m_modelIconTexture->GetID()));
			}
			--m_previewBudget;
			try {
				handle = GetAssetManager().Load<Rendering::Model>(path);
				auto* model = GetAssetManager().Get(handle);
				if (model) {
					auto& preview  = m_modelPreviews[handle.GetID()];
					preview.width  = static_cast<int>(MODEL_PREVIEW_SIZE);
					preview.height = static_cast<int>(MODEL_PREVIEW_SIZE);
					preview.initialized = false;
					preview.Render(model, GetRenderer().GetModelPreviewShader());
					if (preview.texture) {
						return reinterpret_cast<void*>(static_cast<intptr_t>(preview.texture));
					}
				}
			} catch (const std::exception& e) {
				GetUI().log->warn("Failed to load model for preview {}: {}", path, e.what());
			}
			m_failedPreviewPaths.insert(path);
			return reinterpret_cast<void*>(static_cast<intptr_t>(GetUI().m_modelIconTexture->GetID()));
		}

		if (extension == ".material") {
			auto handle = GetAssetManager().GetHandleFromPath<Material>(path);
			if (handle.IsValid()) {
				auto it = m_materialPreviews.find(handle.GetID());
				if (it != m_materialPreviews.end() && it->second.texture) {
					return reinterpret_cast<void*>(static_cast<intptr_t>(it->second.texture));
				}
			}
			if (!allowLoad || m_failedPreviewPaths.count(path) || m_previewBudget <= 0) {
				return reinterpret_cast<void*>(static_cast<intptr_t>(GetUI().m_materialIconTexture->GetID()));
			}
			--m_previewBudget;
			try {
				handle = GetAssetManager().Load<Material>(path);
				auto* mat = GetAssetManager().Get(handle);
				if (mat) {
					auto& preview  = m_materialPreviews[handle.GetID()];
					preview.width  = static_cast<int>(MATERIAL_PREVIEW_SIZE);
					preview.height = static_cast<int>(MATERIAL_PREVIEW_SIZE);
					preview.initialized = false;
					preview.Render(mat, GetRenderer().GetMaterialPreviewShader());
					if (preview.texture) {
						return reinterpret_cast<void*>(static_cast<intptr_t>(preview.texture));
					}
				}
			} catch (const std::exception& e) {
				GetUI().log->warn("Failed to load material for preview {}: {}", path, e.what());
			}
			m_failedPreviewPaths.insert(path);
			return reinterpret_cast<void*>(static_cast<intptr_t>(GetUI().m_materialIconTexture->GetID()));
		}
		else if (extension == ".wav" || extension == ".mp3" || extension == ".ogg") {
			return reinterpret_cast<void*>(static_cast<intptr_t>(GetUI().m_audioIconTexture->GetID()));
		}
		else if (extension == ".anim") {
			return reinterpret_cast<void*>(static_cast<intptr_t>(GetUI().m_animationIconTexture->GetID()));
		}
		else if (extension == ".ozz") {
			// Skeleton vs animation clip vs skinned mesh — pick icon from meta when present.
			const std::string metaPath = path + ".meta";
			if (fs::exists(metaPath)) {
				try {
					std::ifstream file(metaPath);
					nlohmann::json j;
					file >> j;
					if (j.contains("type")) {
						const std::string type = j["type"].get<std::string>();
						if (type.find("Skeleton") != std::string::npos && GetUI().m_skeletonIconTexture) {
							return reinterpret_cast<void*>(static_cast<intptr_t>(GetUI().m_skeletonIconTexture->GetID()));
						}
						if (type.find("Animation") != std::string::npos) {
							return reinterpret_cast<void*>(static_cast<intptr_t>(GetUI().m_animationIconTexture->GetID()));
						}
					}
				} catch (...) {
				}
			}
			const fs::path p(path);
			const std::string name = p.filename().string();
			if ((name.find("skeleton") != std::string::npos || name.find("skel") != std::string::npos) && GetUI().m_skeletonIconTexture) {
				return reinterpret_cast<void*>(static_cast<intptr_t>(GetUI().m_skeletonIconTexture->GetID()));
			}
			return reinterpret_cast<void*>(static_cast<intptr_t>(GetUI().m_fileIconTexture->GetID()));
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
		else if (extension == ".prefab") {
			return reinterpret_cast<void*>(static_cast<intptr_t>(GetUI().m_modelIconTexture->GetID()));
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
				                     fileEntry.extension == ".jpeg" || fileEntry.extension == ".dds" ||
				                     fileEntry.extension == ".obj" || fileEntry.extension == ".material" ||
				                     fileEntry.extension == ".wav" || fileEntry.extension == ".mp3" ||
				                     fileEntry.extension == ".ogg" || fileEntry.extension == ".anim" ||
				                     fileEntry.extension == ".bin" || fileEntry.extension == ".efk" ||
				                     fileEntry.extension == ".ozz");
				
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

	void AssetUIRenderer::DelFile(const std::string& path)
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
		} else if (ext == ".ozz") {
			auto skel = GetAssetManager().GetHandleFromPath<Skeleton>(path);
			if (skel.IsValid()) GetAssetManager().Unload(skel);
			auto anim = GetAssetManager().GetHandleFromPath<Animation>(path);
			if (anim.IsValid()) GetAssetManager().Unload(anim);
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
		InvalidatePreview(path);
		QueueRefresh();
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
		QueueRefresh();
	}

	static void RenameSidecarMeta(const std::string& oldPath, const std::string& newPath)
	{
		const std::string oldMeta = oldPath + ".meta";
		const std::string newMeta = newPath + ".meta";
		if (!fs::exists(oldMeta)) {
			return;
		}
		std::error_code ec;
		if (fs::exists(newMeta)) {
			fs::remove(newMeta, ec);
		}
		fs::rename(oldMeta, newMeta, ec);
		if (ec) {
			GetUI().log->error("Failed to rename meta: {} -> {} ({})", oldMeta, newMeta, ec.message());
		}
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
		fs::rename(oldPath, newPath, ec);
		if (ec) {
			GetUI().log->error("Failed to rename file: {} - {}", oldPath, ec.message());
			return;
		}

		RenameSidecarMeta(oldPath, newPath);

		const std::string ext = fs::path(newPath).extension().string();
		if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".dds" || ext == ".tga") {
			GetAssetManager().RenameAsset<Texture>(oldPath, newPath);
		} else if (ext == ".obj") {
			GetAssetManager().RenameAsset<Rendering::Model>(oldPath, newPath);
		}

		if (m_selectedFile == oldPath) {
			m_selectedFile = newPath;
		}
		m_rightClickedFile = newPath;

		GetUI().log->info("Renamed: {} -> {}", oldPath, newPath);
		InvalidatePreview(oldPath);
		QueueRefresh();
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