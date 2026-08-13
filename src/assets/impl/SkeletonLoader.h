#pragma once

#include "assets/IAssetLoader.h"
#include "animation/Skeleton.h"

namespace Engine {

	class SkeletonLoader : public IAssetLoader<Skeleton> {
	  public:
		std::unique_ptr<Skeleton> LoadFromFile(const std::string& path) override;
	};

} // namespace Engine
