#pragma once
#include "efsw/efsw.hpp"
#include "core/EngineData.h"
#include "assets/AssetManager.h"
#include <string>
#include <filesystem>
#include <iostream>

namespace Engine {

	class HotReloadWatcher : public efsw::FileWatchListener {
	  public:
		void handleFileAction(efsw::WatchID watchid, const std::string& dir, const std::string& filename, efsw::Action action, std::string oldFilename) override
		{
			std::string path = dir + filename;
			
			try {
				std::filesystem::path p(path);
				std::filesystem::path cwd = std::filesystem::current_path();
				std::filesystem::path relative = std::filesystem::relative(p, cwd);
				
				if (action == efsw::Actions::Modified) {
					GetAssetManager().QueueAction({AssetManager::ActionType::Reload, relative.string()});
				}
				else if (action == efsw::Actions::Add) {
					GetAssetManager().QueueAction({AssetManager::ActionType::Load, relative.string()});
				}
				else if (action == efsw::Actions::Delete) {
					GetAssetManager().QueueAction({AssetManager::ActionType::Unload, relative.string()});
				}
				else if (action == efsw::Actions::Moved) {
					std::filesystem::path oldP(dir + oldFilename);
					std::filesystem::path oldRelative = std::filesystem::relative(oldP, cwd);
					GetAssetManager().QueueAction({AssetManager::ActionType::Rename, oldRelative.string(), relative.string()});
				}

			} catch (const std::exception& e) {
				Logger::get("core")->error("Error handling file change: {}", e.what());
			}
		}
	};

} // namespace Engine
