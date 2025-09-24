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
#ifndef EMSCRIPTEN
#include "efsw/efsw.hpp"
#endif
namespace Engine {

#ifndef EMSCRIPTEN
	class AssetWatcher : public efsw::FileWatchListener {
	  public:
		void handleFileAction(efsw::WatchID watchid, const std::string& dir, const std::string& filename, efsw::Action action, std::string oldFilename) override;
	};
#endif

	class AssetUIRenderer {
	  public:
		AssetUIRenderer();

		void RenderAssetWindow();

		void DrawTerrainAssets();
		void DrawSoundAssets();
		void DrawTextureAssets();
		void DrawModelAssets();
		void DrawMaterialAssets();
		void DrawAnimationAssets();


		static bool SelectableBackground(ImVec2 textSize, std::string id, const char* type, const char* typeName);

#ifndef EMSCRIPTEN
		AssetWatcher      listener;
		efsw::FileWatcher fw;
#endif

	  private:
		std::unordered_map<std::type_index, std::function<void()>> drawFuncs;

		std::unordered_map<std::string, ModelPreview> m_modelPreviews;
	};
} // namespace Engine

#endif // CPP_ENGINE_ASSETUIRENDERER_H
