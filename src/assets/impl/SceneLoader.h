//
// Created by gabe on 8/16/25.
//

#ifndef CPP_ENGINE_SCENELOADER_H
#define CPP_ENGINE_SCENELOADER_H

#include "assets/IAssetLoader.h"
#include "core/Scene.h"


namespace Engine {


	class SceneLoader : public IAssetLoader<Scene> {
	  public:
		std::unique_ptr<Scene> LoadFromFile(const std::string& path) override;
	};
} // namespace Engine

#endif // CPP_ENGINE_SCENELOADER_H
