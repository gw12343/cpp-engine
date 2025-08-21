//
// Created by gabe on 8/16/25.
//

#include "SceneManager.h"

#include <utility>
#include "physics/PhysicsManager.h"
#include "scripting/ScriptManager.h"
namespace Engine {

	// Module overrides
	void SceneManager::onInit()
	{
		log = spdlog::stdout_color_mt("SceneManager");
		log->info("SceneManager initialized.");
	}

	void SceneManager::onUpdate(float dt)
	{
		//		if (m_activeScene) {
		//			// You could later call scene-specific update hooks here
		//			log->debug("Updating scene '{}', dt={}", m_activeScene->GetName(), dt);
		//		}
	}


	void SceneManager::onShutdown()
	{
		log->info("SceneManager shutting down. Clearing scenes.");
		//		m_scenes.clear();
		//		m_activeScene.reset();
	}


	// Scene management
	std::unique_ptr<Scene> SceneManager::CreateScene(const std::string& name)
	{
		auto scene = std::make_unique<Scene>(name);
		// m_scenes[name] = scene;
		log->info("Created scene '{}'", name);
		return scene;
	}

	void SceneManager::SetActiveScene(AssetHandle<Scene> scene)
	{
		m_activeScene = std::move(scene);
		GetScriptManager().pendingCollisions.clear();
		GetPhysics().bodyToEntityMap.clear();
	}

	//	void SceneManager::RemoveScene(const std::string& name)
	//	{
	//		if (m_scenes.erase(name) > 0) {
	//			log->info("Removed scene '{}'", name);
	//			if (m_activeScene && m_activeScene->GetName() == name) {
	//				m_activeScene.reset();
	//			}
	//		}
	//	}
	//
	//	std::shared_ptr<Scene> SceneManager::GetScene(const std::string& name)
	//	{
	//		if (auto it = m_scenes.find(name); it != m_scenes.end()) {
	//			return it->second;
	//		}
	//		return nullptr;
	//	}
	//
	//	void SceneManager::SetActiveScene(const std::string& name)
	//	{
	//		if (auto scene = GetScene(name)) {
	//			m_activeScene = scene;
	//			log->info("Active scene set to '{}'", name);
	//		}
	//		else {
	//			log->warn("Scene '{}' not found, cannot set active scene.", name);
	//		}
	//	}
} // namespace Engine
