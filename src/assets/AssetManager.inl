#include "AssetManager.h"

#include <typeinfo>
#include <filesystem>
#include <fstream>

#include <nlohmann/json.hpp>


namespace Engine {
	template <typename T>
	AssetHandle<T> AssetManager::Load(const std::string& path)
	{
		auto& storage = GetStorage<T>();

		// Load or create .meta file
		std::string guid = EnsureMetaFile<T>(path);

		auto it = storage.guidToAsset.find(guid);
		if (it != storage.guidToAsset.end()) return AssetHandle<T>(guid);

		assert(storage.loader);
		auto asset = storage.loader->LoadFromFile(path);
		if (!asset) return AssetHandle<T>();

		storage.guidToAsset[guid] = std::move(asset);
		return AssetHandle<T>(guid);
	}

	template <typename T>
	T* AssetManager::Get(const AssetHandle<T>& handle)
	{
		auto&              storage = GetStorage<T>();
		const std::string& guid    = handle.GetID();

		auto it = storage.guidToAsset.find(guid);
		if (it == storage.guidToAsset.end()) return nullptr;

		return it->second.get();
	}

	template <typename T>
	void AssetManager::RegisterLoader(std::unique_ptr<IAssetLoader<T>> loader)
	{
		auto& storage  = GetStorage<T>();
		storage.loader = std::move(loader);
	}

	template <typename T>
	typename AssetManager::AssetStorage<T>& AssetManager::GetStorage()
	{
		auto typeId = std::type_index(typeid(T));
		if (storages.find(typeId) == storages.end()) {
			storages[typeId] = std::make_unique<AssetStorage<T>>();
		}
		return *static_cast<AssetStorage<T>*>(storages[typeId].get());
	}


	template <typename T>
	std::string AssetManager::EnsureMetaFile(const std::string& assetPath)
	{
		std::string metaPath = assetPath + ".meta";

		if (std::filesystem::exists(metaPath)) {
			// Load existing
			std::ifstream  file(metaPath);
			nlohmann::json j;
			file >> j;
			return j["guid"];
		}
		else {
			// Generate new GUID
			std::string guid = GenerateGUID();

			// Save
			nlohmann::json j;
			j["guid"] = guid;
			j["type"] = typeid(T).name();

			std::ofstream file(metaPath);
			file << j.dump(4);
			return guid;
		}
	}

} // namespace Engine