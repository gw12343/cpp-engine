//
// Created by gabe on 6/30/25.
//

#ifndef CPP_ENGINE_ASSETMANAGER_H
#define CPP_ENGINE_ASSETMANAGER_H

#include <unordered_map>
#include <typeindex>
#include <memory>
#include <string>
#include <cassert>

#include "AssetHandle.h"
#include "IAssetLoader.h"

#include "rendering/ui/UIManager.h"

namespace Engine {

	class AssetManager {
	  public:
		template <typename T>
		AssetHandle<T> Load(const std::string& path);

		template <typename T>
		T* Get(const AssetHandle<T>& handle);

		template <typename T>
		void RegisterLoader(std::unique_ptr<IAssetLoader<T>> loader);

	  private:
		struct IStorageBase {
			virtual ~IStorageBase() = default;
		};

		template <typename T>
		struct AssetStorage : IStorageBase {
			std::unordered_map<std::string, AssetHandle<T>>  pathToHandle;
			std::unordered_map<uint32_t, std::unique_ptr<T>> assets;
			std::unique_ptr<IAssetLoader<T>>                 loader;
			uint32_t                                         nextID = 1;
		};

		template <typename T>
		AssetStorage<T>& GetStorage();

		std::unordered_map<std::type_index, std::unique_ptr<IStorageBase>> storages;

		friend class UI::UIManager;
	};

} // namespace Engine

#endif // CPP_ENGINE_ASSETMANAGER_H
