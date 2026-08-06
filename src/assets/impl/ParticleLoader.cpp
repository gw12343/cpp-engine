//
// Created by gabe on 8/14/25.
//

#include "ParticleLoader.h"



namespace Engine {
	std::unique_ptr<Particle> ParticleLoader::LoadFromFile(const std::string& path)
	{
		auto particle = std::make_unique<Particle>();

		if (!particle->LoadFromFile(path)) {
			return nullptr;
		}


		return particle;
	}

	bool ParticleLoader::Reload(Particle& asset, const std::string& path)
	{
		return asset.LoadFromFile(path);
	}
} // namespace Engine