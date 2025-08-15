//
// Created by gabe on 8/14/25.
//
#pragma once

#include "assets/IAssetLoader.h"
#include "sound/SoundManager.h"


#include <string>
#include <unordered_map>

namespace Engine {
	class SoundLoader : public IAssetLoader<Audio::SoundBuffer> {
	  public:
		std::unique_ptr<Audio::SoundBuffer> LoadFromFile(const std::string& path) override;
	};
} // namespace Engine