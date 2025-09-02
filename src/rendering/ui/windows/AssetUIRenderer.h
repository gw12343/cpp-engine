//
// Created by gabe on 8/17/25.
//

#ifndef CPP_ENGINE_ASSETUIRENDERER_H
#define CPP_ENGINE_ASSETUIRENDERER_H

#include <unordered_map>
#include <typeindex>
#include <functional>
#include <string>

#include "rendering/ui/ModelPreview.h"
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

		void DrawTerrainAssets();
		void DrawSoundAssets();
		void DrawTextureAssets();
		void DrawModelAssets();
		void DrawMaterialAssets();


		static bool SelectableBackground(ImVec2 textSize, std::string id, const char* type, const char* typeName);

		AssetWatcher listener;

		efsw::FileWatcher fw;

	  private:
		std::unordered_map<std::type_index, std::function<void()>> drawFuncs;

		std::unordered_map<std::string, ModelPreview> m_modelPreviews;
	};
} // namespace Engine

#endif // CPP_ENGINE_ASSETUIRENDERER_H
