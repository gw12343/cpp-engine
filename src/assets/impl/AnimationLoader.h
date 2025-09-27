//
// Created by gabe on 9/10/25.
//

#pragma once

#include "assets/IAssetLoader.h"
#include "rendering/Texture.h"

#include "ozz/animation/runtime/animation.h"
#include "animation/Animation.h"

namespace Engine {


	class AnimationLoader : public IAssetLoader<Animation> {
	  public:
		std::unique_ptr<Animation> LoadFromFile(const std::string& path) override;
	};
} // namespace Engine
