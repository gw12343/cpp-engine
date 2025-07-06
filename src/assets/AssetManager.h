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
#include <random>

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

		struct IStorageBase {
			virtual ~IStorageBase() = default;
		};

		template <typename T>
		struct AssetStorage : IStorageBase {
			std::unordered_map<std::string, std::unique_ptr<T>> guidToAsset;
			std::unique_ptr<IAssetLoader<T>>                    loader;
		};


		template <typename T>
		AssetStorage<T>& GetStorage();

		template <typename T>
		std::string EnsureMetaFile(const std::string& assetPath);


	  private:
		static std::string GenerateGUID()
		{
			std::stringstream               ss;
			std::random_device              rd;
			std::mt19937                    gen(rd());
			std::uniform_int_distribution<> dis(0, 15);

			for (int i = 0; i < 32; ++i)
				ss << std::hex << dis(gen);
			return ss.str();
		}


		std::unordered_map<std::type_index, std::unique_ptr<IStorageBase>> storages;

		friend class UI::UIManager;
	};

} // namespace Engine

#endif // CPP_ENGINE_ASSETMANAGER_H
