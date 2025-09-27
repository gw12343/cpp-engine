//
// Created by gabe on 8/16/25.
//

#include "SceneManager.h"

#include <utility>
#include <tracy/Tracy.hpp>
#include "physics/PhysicsManager.h"
#include "scripting/ScriptManager.h"
#include "components/impl/RigidBodyComponent.h"
#include "assets/AssetManager.h"
namespace Engine {

	// Module overrides
	void SceneManager::onInit()
	{
		log = spdlog::stdout_color_mt("SceneManager");
		log->info("SceneManager initialized.");
	}

	void SceneManager::onUpdate(float dt)
	{
		ZoneScoped;
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
		GetScriptManager().pendingCharacterCollisions.clear();

		auto& physics = GetPhysics();
		physics.bodyToEntityMap.clear();

		auto   physicsView = GetCurrentSceneRegistry().view<Components::RigidBodyComponent>();
		Scene* s           = GetAssetManager().Get(m_activeScene);
		for (auto [entity, rb] : physicsView.each()) {
			physics.bodyToEntityMap[rb.bodyID] = Entity(entity, s);
		}
	}
} // namespace Engine

#include "assets/AssetManager.inl"
