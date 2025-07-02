//
// Created by gabe on 7/1/25.
//

#include <fstream>
#include "TerrainLoader.h"

namespace Engine {


	std::unique_ptr<Terrain::TerrainTile> TerrainLoader::LoadFromFile(const std::string& path)
	{
		auto tile = std::make_unique<Terrain::TerrainTile>();

		std::ifstream file(path, std::ios::binary);
		if (!file) throw std::runtime_error("Cannot open terrain file");

		char magic[4];
		file.read(magic, 4);
		if (std::string(magic, 4) != "TERR") throw std::runtime_error("Invalid format");

		uint32_t version;
		file.read(reinterpret_cast<char*>(&version), sizeof(uint32_t));
		if (version < 2 || version > 3) throw std::runtime_error("Unsupported terrain version");


		uint32_t nameLen;
		file.read(reinterpret_cast<char*>(&nameLen), sizeof(uint32_t));
		tile->name.resize(nameLen);
		file.read(tile->name.data(), nameLen);

		file.read(reinterpret_cast<char*>(&tile->heightRes), sizeof(uint32_t));
		file.read(reinterpret_cast<char*>(&tile->splatRes), sizeof(uint32_t));
		file.read(reinterpret_cast<char*>(&tile->sizeX), sizeof(float));
		file.read(reinterpret_cast<char*>(&tile->sizeY), sizeof(float));
		file.read(reinterpret_cast<char*>(&tile->sizeZ), sizeof(float));

		if (version >= 3) {
			file.read(reinterpret_cast<char*>(&tile->posX), sizeof(float));
			file.read(reinterpret_cast<char*>(&tile->posY), sizeof(float));
			file.read(reinterpret_cast<char*>(&tile->posZ), sizeof(float));
			spdlog::debug("loaded terrain chunk at ({}, {}, {})", tile->posX, tile->posY, tile->posZ);
		}
		else {
			spdlog::debug("unknown position, assuming (0, 0, 0).");
		}

		file.read(reinterpret_cast<char*>(&tile->splatLayerCount), sizeof(uint32_t));

		size_t heightCount = tile->heightRes * tile->heightRes;
		tile->heightmap.resize(heightCount);
		file.read(reinterpret_cast<char*>(tile->heightmap.data()), static_cast<std::streamsize>(heightCount * sizeof(float)));


		size_t splatCount = tile->splatRes * tile->splatRes * tile->splatLayerCount;
		tile->splatmap.resize(splatCount);
		file.read(reinterpret_cast<char*>(tile->splatmap.data()), static_cast<std::streamsize>(splatCount));

		uint32_t treeCount;
		file.read(reinterpret_cast<char*>(&treeCount), sizeof(uint32_t));
		tile->trees.resize(treeCount);
		for (auto& tree : tile->trees) {
			file.read(reinterpret_cast<char*>(&tree.x), sizeof(float));
			file.read(reinterpret_cast<char*>(&tree.y), sizeof(float));
			file.read(reinterpret_cast<char*>(&tree.z), sizeof(float));
			file.read(reinterpret_cast<char*>(&tree.scale), sizeof(float));
			file.read(reinterpret_cast<char*>(&tree.prefabIndex), sizeof(uint32_t));
		}

		return tile;
	}
} // namespace Engine