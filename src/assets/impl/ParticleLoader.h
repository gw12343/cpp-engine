//
// Created by gabe on 8/14/25.
//
#pragma once

#include "assets/IAssetLoader.h"
#include "rendering/particles/Particle.h"



#include <unordered_map>

namespace Engine {
	class ParticleLoader : public IAssetLoader<Particle> {
	  public:
		std::unique_ptr<Particle> LoadFromFile(const std::string& path) override;
		bool                      Reload(Particle& asset, const std::string& path) override;
	};
} // namespace Engine