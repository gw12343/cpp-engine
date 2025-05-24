// #include "TerrainLoader.h"
//
// #include "spdlog/spdlog.h"
//
// namespace Engine {
//	namespace Terrain {
//
//		TerrainTile LoadTerrainTile(const std::string& path)
//		{
//			spdlog::debug("loading file!");
//			TerrainTile   terrain;
//			std::ifstream file(path, std::ios::binary);
//			if (!file) throw std::runtime_error("Cannot open terrain file.");
//
//			char magic[4];
//			file.read(magic, 4);
//			if (std::string(magic, 4) != "TERR") throw std::runtime_error("Invalid terrain file format");
//
//			uint32_t version;
//			file.read(reinterpret_cast<char*>(&version), sizeof(uint32_t));
//			spdlog::debug("version: {}", version);
//			if (version != 2) throw std::runtime_error("Unsupported terrain file version");
//
//			// Read tile name
//			uint32_t nameLen;
//			file.read(reinterpret_cast<char*>(&nameLen), sizeof(uint32_t));
//			terrain.name.resize(nameLen);
//			file.read(terrain.name.data(), nameLen);
//			spdlog::debug("terrain name: {}", terrain.name);
//
//			// Read resolution + size
//			file.read(reinterpret_cast<char*>(&terrain.heightRes), sizeof(uint32_t));
//			file.read(reinterpret_cast<char*>(&terrain.splatRes), sizeof(uint32_t));
//			file.read(reinterpret_cast<char*>(&terrain.sizeX), sizeof(float));
//			file.read(reinterpret_cast<char*>(&terrain.sizeY), sizeof(float));
//			file.read(reinterpret_cast<char*>(&terrain.sizeZ), sizeof(float));
//			spdlog::debug("height res: {}", terrain.heightRes);
//			spdlog::debug("splat res: {}", terrain.splatRes);
//			spdlog::debug("size x: {}", terrain.sizeX);
//			spdlog::debug("size y: {}", terrain.sizeY);
//			spdlog::debug("size z: {}", terrain.sizeZ);
//
//
//			// Read splat layer count
//			uint32_t splatLayerCount;
//			file.read(reinterpret_cast<char*>(&splatLayerCount), sizeof(uint32_t));
//
//			spdlog::debug("got splat layer count: {}", splatLayerCount);
//			// Heightmap
//			size_t heightCount = terrain.heightRes * terrain.heightRes;
//			terrain.heightmap.resize(heightCount);
//			file.read(reinterpret_cast<char*>(terrain.heightmap.data()), heightCount * sizeof(float));
//
//			spdlog::debug("loaded heightmap data: {}", heightCount);
//
//			// Splatmap
//			size_t splatCount = terrain.splatRes * terrain.splatRes * splatLayerCount;
//			terrain.splatmap.resize(splatCount);
//			file.read(reinterpret_cast<char*>(terrain.splatmap.data()), splatCount);
//
//			spdlog::debug("loaded splatmap data: {} with {} layers", splatCount, splatLayerCount);
//
//			// Read trees
//			uint32_t treeCount;
//			file.read(reinterpret_cast<char*>(&treeCount), sizeof(uint32_t));
//			terrain.trees.resize(treeCount);
//			for (auto& tree : terrain.trees) {
//				file.read(reinterpret_cast<char*>(&tree.x), sizeof(float));
//				file.read(reinterpret_cast<char*>(&tree.y), sizeof(float));
//				file.read(reinterpret_cast<char*>(&tree.z), sizeof(float));
//				file.read(reinterpret_cast<char*>(&tree.scale), sizeof(float));
//				file.read(reinterpret_cast<char*>(&tree.prefabIndex), sizeof(uint32_t));
//			}
//
//			return terrain;
//		}
//
//	} // namespace Terrain
// } // namespace Engine