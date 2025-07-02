//
// Created by gabe on 7/1/25.
//

#ifndef CPP_ENGINE_TERRAINLOADER_H
#define CPP_ENGINE_TERRAINLOADER_H

#include "terrain/TerrainManager.h"
#include "assets/IAssetLoader.h"
namespace Engine {

	class TerrainLoader : public IAssetLoader<Terrain::TerrainTile> {
	  public:
		std::unique_ptr<Terrain::TerrainTile> LoadFromFile(const std::string& path) override;
	};

} // namespace Engine

#endif // CPP_ENGINE_TERRAINLOADER_H
