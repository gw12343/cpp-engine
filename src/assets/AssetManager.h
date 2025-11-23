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
#include <mutex>
#include <vector>

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
		void Unload(const AssetHandle<T>& handle);

		template <typename T>
		void RegisterLoader(std::unique_ptr<IAssetLoader<T>> loader);

		template <typename T>
		void Reload(const AssetHandle<T>& handle);

		template <typename T>
		void UnloadByPath(const std::string& path);

		template <typename T>
		void RenameAsset(const std::string& oldPath, const std::string& newPath);

		template <typename T>
		AssetHandle<T> GetHandleFromPath(const std::string& path);

		enum class ActionType { Reload, Load, Unload, Rename };
		struct AssetAction {
			ActionType  type;
			std::string path;
			std::string newPath; // For Rename
		};

		void QueueAction(const AssetAction& action);
		void Update();

		struct IStorageBase {
			virtual ~IStorageBase() = default;
		};

		template <typename T>
		struct AssetStorage : IStorageBase {
			std::unordered_map<std::string, std::unique_ptr<T>> guidToAsset;
			std::unordered_map<std::string, std::string>        pathToGuid; // New: Map path to GUID
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
		
		std::vector<AssetAction> m_pendingActions;
		std::mutex               m_actionMutex;

		friend class UI::UIManager;
	};

} // namespace Engine

#include "AssetManager.inl"

#endif // CPP_ENGINE_ASSETMANAGER_H
