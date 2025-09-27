//
// Created by gabe on 6/30/25.
//

#include "AnimationLoader.h"
#include "core/EngineData.h"
#include "animation/AnimationManager.h"

#include <iostream>

namespace Engine {
	std::unique_ptr<Animation> AnimationLoader::LoadFromFile(const std::string& path)
	{
		std::unique_ptr<Animation> animation       = std::make_unique<Animation>();
		ozz::animation::Animation* animationSource = GetAnimationManager().LoadAnimationFromPath(path);
		if (!animation) {
			spdlog::error("Failed to load animation from path: {}", path);
		}
		else {
			GetDefaultLogger()->info("Loaded animation from path: {}", path);
		}
		animation->name   = path.substr(path.find_last_of("/\\") + 1);
		animation->source = animationSource;
		return animation;
	}

} // namespace Engine