#ifndef CPP_ENGINE_ASSETMANAGER_INL
#define CPP_ENGINE_ASSETMANAGER_INL

//#include "AssetManager.h"

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
		storage.pathToGuid[path]  = guid;
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
	void AssetManager::Unload(const AssetHandle<T>& handle)
	{
		auto& storage = GetStorage<T>();
		auto  it      = storage.guidToAsset.find(handle.GetID());
		if (it != storage.guidToAsset.end()) {
			storage.guidToAsset.erase(it); // deletes the asset, unique_ptr cleans it up
		}
	}

	template <typename T>
	void AssetManager::Reload(const AssetHandle<T>& handle)
	{
		auto&              storage = GetStorage<T>();
		const std::string& guid    = handle.GetID();

		auto it = storage.guidToAsset.find(guid);
		if (it == storage.guidToAsset.end()) return;

		// Find path from guid (reverse lookup needed or store it)
		// Since we store path->guid, we can iterate or store guid->path.
		// For now, let's iterate since reload is rare.
		std::string path;
		for (const auto& [p, g] : storage.pathToGuid) {
			if (g == guid) {
				path = p;
				break;
			}
		}

		if (!path.empty()) {
			if (storage.loader->Reload(*it->second, path)) {
				Logger::get("core")->info("Reloaded asset: {}", path);
			} else {
				Logger::get("core")->error("Failed to reload asset: {}", path);
			}
		}
	}

	template <typename T>
	AssetHandle<T> AssetManager::GetHandleFromPath(const std::string& path)
	{
		auto& storage = GetStorage<T>();
		auto  it      = storage.pathToGuid.find(path);
		if (it != storage.pathToGuid.end()) {
			return AssetHandle<T>(it->second);
		}
		return AssetHandle<T>();
	}

	template <typename T>
	void AssetManager::UnloadByPath(const std::string& path)
	{
		auto& storage = GetStorage<T>();
		auto  it      = storage.pathToGuid.find(path);
		if (it != storage.pathToGuid.end()) {
			std::string guid = it->second;
			storage.guidToAsset.erase(guid);
			storage.pathToGuid.erase(it);
			Logger::get("core")->info("Unloaded asset: {}", path);
		}
	}

	template <typename T>
	void AssetManager::RenameAsset(const std::string& oldPath, const std::string& newPath)
	{
		auto& storage = GetStorage<T>();
		auto  it      = storage.pathToGuid.find(oldPath);
		if (it != storage.pathToGuid.end()) {
			std::string guid = it->second;
			storage.pathToGuid.erase(it);
			storage.pathToGuid[newPath] = guid;
			// Rename .meta file
			std::string oldMetaPath = oldPath + ".meta";
			std::string newMetaPath = newPath + ".meta";
			if (std::filesystem::exists(oldMetaPath)) {
				std::error_code ec;
				std::filesystem::rename(oldMetaPath, newMetaPath, ec);
				if (ec) {
					Logger::get("core")->error("Failed to rename meta file: {} -> {}", oldMetaPath, newMetaPath);
				}
			}
			
			// Update asset's internal name if it has one
			auto assetIt = storage.guidToAsset.find(guid);
			if (assetIt != storage.guidToAsset.end() && assetIt->second) {
				std::filesystem::path p(newPath);
				std::string newName = p.filename().string();
				
				// Try to call SetName if it exists, otherwise set m_name directly
				// This works for types like Texture that have SetName
				auto* ptr = assetIt->second.get();
				if constexpr (std::is_same_v<T, Texture>) {
					static_cast<Texture*>(ptr)->SetName(newName);
				}
				// For types with public m_name (like Model)
				else {
					ptr->m_name = newName;
				}
			}
			
			Logger::get("core")->info("Renamed asset: {} -> {}", oldPath, newPath);
		}
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

#endif // CPP_ENGINE_ASSETMANAGER_INL