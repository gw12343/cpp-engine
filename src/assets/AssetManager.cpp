#include "AssetManager.h"
#include "rendering/Renderer.h"
#include "core/EngineData.h"

namespace Engine {

	void AssetManager::QueueAction(const AssetAction& action)
	{
		std::lock_guard<std::mutex> lock(m_actionMutex);
		m_pendingActions.push_back(action);
	}

	void AssetManager::Update()
	{
		std::vector<AssetAction> actions;
		{
			std::lock_guard<std::mutex> lock(m_actionMutex);
			actions = std::move(m_pendingActions);
			m_pendingActions.clear();
		}

		for (const auto& action : actions) {
			std::string ext = std::filesystem::path(action.path).extension().string();

			if (action.type == ActionType::Reload) {
				if (ext == ".png" || ext == ".jpg" || ext == ".hdr") {
					auto handle = GetHandleFromPath<Texture>(action.path);
					if (handle.IsValid()) Reload(handle);
				}
				else if (ext == ".obj" || ext == ".gltf" || ext == ".fbx") {
					auto handle = GetHandleFromPath<Rendering::Model>(action.path);
					if (handle.IsValid()) Reload(handle);
				}
				else if (ext == ".glsl" || ext == ".vert" || ext == ".frag") {
					GetRenderer().ReloadShaders();
				}
			}
			else if (action.type == ActionType::Load) {
				if (ext == ".png" || ext == ".jpg" || ext == ".hdr") {
					Load<Texture>(action.path);
				}
				else if (ext == ".obj" || ext == ".gltf" || ext == ".fbx") {
					Load<Rendering::Model>(action.path);
				}
			}
			else if (action.type == ActionType::Unload) {
				if (ext == ".png" || ext == ".jpg" || ext == ".hdr") {
					UnloadByPath<Texture>(action.path);
				}
				else if (ext == ".obj" || ext == ".gltf" || ext == ".fbx") {
					UnloadByPath<Rendering::Model>(action.path);
				}
			}
			else if (action.type == ActionType::Rename) {
				if (ext == ".png" || ext == ".jpg" || ext == ".hdr") {
					RenameAsset<Texture>(action.path, action.newPath);
				}
				else if (ext == ".obj" || ext == ".gltf" || ext == ".fbx") {
					RenameAsset<Rendering::Model>(action.path, action.newPath);
				}
			}
		}
	}

} // namespace Engine
