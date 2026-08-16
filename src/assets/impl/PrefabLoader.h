#pragma once

#include "assets/IAssetLoader.h"
#include "assets/Prefab.h"

namespace Engine {
	class PrefabLoader : public IAssetLoader<Prefab> {
	  public:
		std::unique_ptr<Prefab> LoadFromFile(const std::string& path) override;
		static bool             SaveToFile(const Prefab& prefab, const std::string& path);
	};
} // namespace Engine
