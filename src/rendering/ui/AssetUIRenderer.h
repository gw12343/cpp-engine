//
// Created by gabe on 8/17/25.
//

#ifndef CPP_ENGINE_ASSETUIRENDERER_H
#define CPP_ENGINE_ASSETUIRENDERER_H

#include <unordered_map>
#include <typeindex>
#include <functional>
#include <string>

#include "ModelPreview.h"


namespace Engine {

	class AssetUIRenderer {
	  public:
		AssetUIRenderer();

		void RenderAssetWindow();

		void DrawTerrainAssets();
		void DrawSoundAssets();
		void DrawTextureAssets();
		void DrawModelAssets();

	  private:
		std::unordered_map<std::type_index, std::function<void()>> drawFuncs;

		std::unordered_map<std::string, ModelPreview> m_modelPreviews;
	};
} // namespace Engine

#endif // CPP_ENGINE_ASSETUIRENDERER_H
