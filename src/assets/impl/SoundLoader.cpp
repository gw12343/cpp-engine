//
// Created by gabe on 8/14/25.
//

#include "SoundLoader.h"

#include <iostream>

namespace Engine {
	std::unique_ptr<Audio::SoundBuffer> SoundLoader::LoadFromFile(const std::string& path)
	{
		auto buffer = std::make_unique<Audio::SoundBuffer>(path);
		if (!buffer->IsLoaded()) {
			return nullptr;
		}

		return buffer;
	}

} // namespace Engine