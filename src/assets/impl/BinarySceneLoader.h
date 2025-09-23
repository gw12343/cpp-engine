//
// Created by gabe on 8/16/25.
//

#ifndef CPP_ENGINE_BINARYSCENELOADER_H
#define CPP_ENGINE_BINARYSCENELOADER_H

#include "assets/IAssetLoader.h"
#include "core/Scene.h"
#include "assets/AssetHandle.h"


namespace Engine {


	class [[maybe_unused]] BinarySceneLoader : public IAssetLoader<Scene> {
	  public:
		std::unique_ptr<Scene>       LoadFromFile(const std::string& path) override;
		[[maybe_unused]] static void SerializeScene(const AssetHandle<Scene>& sceneRef, const std::string& path);
	};
} // namespace Engine

#endif // CPP_ENGINE_BINARYSCENELOADER_H
