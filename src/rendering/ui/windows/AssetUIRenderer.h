//
// Created by gabe on 8/17/25.
//

#ifndef CPP_ENGINE_ASSETUIRENDERER_H
#define CPP_ENGINE_ASSETUIRENDERER_H

#include <unordered_map>
#include <unordered_set>
#include <typeindex>
#include <functional>
#include <string>

#include "rendering/ui/ModelPreview.h"
#include "rendering/ui/MaterialPreview.h"
#include "imgui.h"
#include "efsw/efsw.hpp"

namespace Engine {
	class AssetWatcher : public efsw::FileWatchListener {
	  public:
		void handleFileAction(efsw::WatchID watchid, const std::string& dir, const std::string& filename, efsw::Action action, std::string oldFilename) override;
	};

	class AssetUIRenderer {
	  public:
		AssetUIRenderer();

		void RenderAssetWindow();

		// File browser rendering methods
		void RenderDirectoryTree();
		void RenderFileGrid();
		void RenderFileCard(const std::string& path, const std::string& filename, bool isDirectory);
		void RenderContextMenu();

		// File operations
		void DelFile(const std::string& path);
		void DuplicateFile(const std::string& path);
		void RenameFile(const std::string& oldPath, const std::string& newPath);
		void ScanDirectory(const std::string& dirPath);
		void RefreshCurrentDirectory();

		// Directory tree traversal
		void RenderDirectoryTreeNode(const std::string& dirPath, const std::string& dirName);

		// Helper to get icon for file type
		void* GetIconForFile(const std::string& path, const std::string& extension, bool isDirectory);

		static bool SelectableBackground(ImVec2 textSize, std::string id, const char* type, const char* typeName);

		AssetWatcher listener;
		efsw::FileWatcher fw;

	  private:
		struct FileEntry {
			std::string path;
			std::string filename;
			std::string extension;
			size_t size;
			bool isDirectory;
			bool isAsset; // true if it's a loadable asset type
		};

		std::string m_currentDirectory;
		std::vector<FileEntry> m_currentFiles;
		std::string m_selectedFile;
		std::string m_rightClickedFile;
		
		// Rename state
		bool m_renamingFile = false;
		char m_renameBuffer[256];
		
		// For previews
		std::unordered_map<std::string, ModelPreview> m_modelPreviews;
		std::unordered_map<std::string, MaterialPreview> m_materialPreviews;
		// Cache to avoid reloading assets every frame
		std::unordered_set<std::string> m_loadedModelPaths;
		std::unordered_set<std::string> m_loadedMaterialPaths;
	};
} // namespace Engine

#endif // CPP_ENGINE_ASSETUIRENDERER_H
