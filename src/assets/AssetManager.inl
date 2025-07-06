#include "AssetManager.h"

#include <typeinfo>

namespace Engine {
	template <typename T>
	AssetHandle<T> AssetManager::Load(const std::string& path)
	{
		auto& storage = GetStorage<T>();
		auto  it      = storage.pathToHandle.find(path);
		if (it != storage.pathToHandle.end()) {
			return it->second;
		}

		assert(storage.loader && "No loader registered for this asset type.");
		auto asset = storage.loader->LoadFromFile(path);
		if (!asset) return AssetHandle<T>();

		uint32_t id        = storage.nextID++;
		storage.assets[id] = std::move(asset);
		AssetHandle<T> handle(id);
		storage.pathToHandle[path] = handle;
		return handle;
	}

	template <typename T>
	T* AssetManager::Get(const AssetHandle<T>& handle)
	{
		auto& storage = GetStorage<T>();
		auto  it      = storage.assets.find(handle.GetID());
		if (it == storage.assets.end()) return nullptr;
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
} // namespace Engine