//
// Created by gabe on 8/16/25.
//

#ifndef CPP_ENGINE_BINARYSCENELOADER_H
#define CPP_ENGINE_BINARYSCENELOADER_H

#include "assets/IAssetLoader.h"
#include "core/Scene.h"


namespace Engine {


	class BinarySceneLoader : public IAssetLoader<Scene> {
	  public:
		std::unique_ptr<Scene> LoadFromFile(const std::string& path) override;
		static void            SerializeScene(AssetHandle<Scene> sceneRef, const std::string& path);
	};
} // namespace Engine

#endif // CPP_ENGINE_BINARYSCENELOADER_H
