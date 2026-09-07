//
// Created by gabe on 8/16/25.
//

#include "SceneManager.h"

#include <utility>

#include "physics/PhysicsManager.h"
#include "scripting/ScriptManager.h"
#include "components/impl/RigidBodyComponent.h"
#include "components/impl/TransformComponent.h"

#include "components/impl/EntityMetadataComponent.h"
#include "core/ProjectSettings.h"
#include "core/EnginePaths.h"
#include "core/ThreadPool.h"
#include "core/EngineData.h"
#include "core/module/ModuleManager.h"
#include "rendering/particles/ParticleManager.h"
#include "rendering/ui/GameUIManager.h"
#ifndef GAME_BUILD
#include "rendering/ui/UIManager.h"
#endif
#include <filesystem>
#include <vector>

namespace Engine {

	// Module overrides
	void SceneManager::onInit()
	{
        ZoneScopedN("Initialize SceneManager");
		log->info("SceneManager initialized.");
	}


	void SceneManager::onUpdate(float dt)
	{
		ZoneScoped;
		UpdateTransforms();
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

	void SceneManager::SetActiveScene(SceneHandle scene)
	{
		m_activeScene = std::move(scene);
		GetScriptManager().pendingCollisions.clear();
		GetScriptManager().pendingCharacterCollisions.clear();

		auto& physics = GetPhysics();
		physics.bodyToEntityMap.clear();

		Scene* s = GetAssetManager().Get(m_activeScene);
		if (!s || !s->GetRegistry()) {
			m_activeBuildIndex = -1;
			return;
		}

		auto physicsView = s->GetRegistry()->view<Components::RigidBodyComponent>();
		for (auto [entity, rb] : physicsView.each()) {
			physics.bodyToEntityMap[rb.bodyID] = Entity(entity, s);
		}
	}

	void SceneManager::UnloadActive()
	{
#ifndef GAME_BUILD
		if (Get().ui) {
			GetUI().m_selectedEntity = Entity();
		}
#endif
		GetParticleManager().ResetInternalManager();
		GetScriptManager().GetEventBus().ClearAllSubscriptions();

		auto&        physics = GetPhysics();
		BodyIDVector outBodies;
		if (physics.GetPhysicsSystem()) {
			physics.GetPhysicsSystem()->GetBodies(outBodies);
			for (auto body : outBodies) {
				if (physics.GetPhysicsSystem()->GetBodyInterface().IsAdded(body)) {
					physics.GetPhysicsSystem()->GetBodyInterface().RemoveBody(body);
				}
			}
		}
		physics.bodyToEntityMap.clear();

		GetGameUIManager().CloseAllDocuments();
		if (m_activeScene.IsValid()) {
			GetAssetManager().Unload<Scene>(m_activeScene);
		}
		m_activeScene      = SceneHandle();
		m_activeBuildIndex = -1;
	}

	bool SceneManager::LoadScenePath(const std::string& path, bool restartScripts)
	{
		if (path.empty() || !GetEnginePaths().Exists(path)) {
			log->error("Scene does not exist: {}", path);
			return false;
		}

		UnloadActive();
		SetActiveScene(GetAssetManager().Load<Scene>(path));
		if (!GetAssetManager().Get(m_activeScene)) {
			log->error("Failed to load scene: {}", path);
			return false;
		}

		m_activeBuildIndex = GetProject().FindSceneIndex(path);
		GetGameUIManager().resetDocuments();

		if (restartScripts && IsSimulating() && Get().manager) {
			Get().manager->StartGame();
		}
		log->info("Loaded scene '{}' (build index {})", path, m_activeBuildIndex);
		return true;
	}

	bool SceneManager::LoadSceneIndex(int index, bool restartScripts)
	{
		const std::string path = GetProject().RuntimeScenePath(index);
		if (path.empty()) {
			log->error("No project scene at index {}", index);
			return false;
		}
		if (!LoadScenePath(path, restartScripts)) {
			return false;
		}
		m_activeBuildIndex = index;
		return true;
	}

	void SceneManager::RequestLoadScene(int index)
	{
		m_hasPendingScene = true;
		m_pendingIndex    = index;
		m_pendingPath.clear();
	}

	void SceneManager::RequestLoadScenePath(const std::string& path)
	{
		m_hasPendingScene = true;
		m_pendingIndex    = -1;
		m_pendingPath     = path;
	}

	void SceneManager::FlushPendingSceneLoad()
	{
		if (!m_hasPendingScene) {
			return;
		}
		const int         index = m_pendingIndex;
		const std::string path  = m_pendingPath;
		m_hasPendingScene       = false;
		m_pendingIndex          = -1;
		m_pendingPath.clear();

		if (index >= 0) {
			LoadSceneIndex(index, true);
		}
		else if (!path.empty()) {
			LoadScenePath(GetProject().NormalizePath(path), true);
		}
	}

	void SceneManager::setLuaBindings()
	{
		auto& lua = GetScriptManager().lua;
		lua.set_function("loadScene",
		                 sol::overload([](int index) { GetSceneManager().RequestLoadScene(index); },
		                               [](const std::string& path) { GetSceneManager().RequestLoadScenePath(path); }));
		lua.set_function("getSceneCount", []() { return GetProject().SceneCount(); });
		lua.set_function("getActiveSceneIndex", []() { return GetSceneManager().GetActiveBuildIndex(); });
		lua.set_function("getScenePath", [](int index) { return GetProject().SourceScenePath(index); });
		lua.set_function("getProjectName", []() { return GetProject().name; });
	}
	void SceneManager::UpdateTransforms()
	{
		// Each root hierarchy is independent — process roots in parallel.
		std::vector<Entity> roots;
		roots.reserve(64);

		auto view = GetCurrentSceneRegistry().view<Components::EntityMetadata, Components::Transform>();
		for (auto [entity, metadata, transform] : view.each()) {
			if (!metadata.parentEntity.IsValid()) {
				roots.emplace_back(entity, GetCurrentScene());
			}
		}

		const int n = static_cast<int>(roots.size());
		GetThreadPool().ParallelForIndex(n, /*minPerTask=*/2, [&](int i) {
			UpdateTransformRecursive(roots[static_cast<size_t>(i)], glm::mat4(1.0f), false);
		});

		// Kinematic / static bodies follow the authored transform (including parented
		// platforms). Dynamic bodies are written the other way in PhysicsManager.
		// Sequential: Jolt BodyInterface is not safe to call from the transform workers.
		if (IsSimulating()) {
			auto kinematicView = GetCurrentSceneRegistry().view<Components::Transform, Components::RigidBodyComponent>();
			for (auto [entity, transform, rb] : kinematicView.each()) {
				if (rb.bodyID.IsInvalid()) continue;
				if (rb.motionType == static_cast<int>(JPH::EMotionType::Dynamic)) continue;
				Entity wrapped(entity, GetCurrentScene());
				transform.SyncWithPhysics(wrapped);
			}
		}
	}
	void SceneManager::UpdateTransformRecursive(Entity entity, const glm::mat4& parentMatrix, bool hasParent)
	{
		if (!entity.HasComponent<Components::Transform>()) return;
		auto& transform = entity.GetComponent<Components::Transform>();

		transform.SetWorldFromMatrix(parentMatrix * transform.GetLocalMatrix());

		// Update children
		auto& hierarchy = entity.GetComponent<Components::EntityMetadata>();
		const std::vector<EntityHandle> children = hierarchy.children;
		for (const auto& childHandle : children) {
			Entity childEntity = entity.m_scene ? entity.m_scene->Get(childHandle) : GetCurrentScene()->Get(childHandle);
			if (childEntity.IsValid()) {
				UpdateTransformRecursive(childEntity, transform.GetWorldMatrix(), true);
			}
		}
	}
} // namespace Engine

#include "assets/AssetManager.inl"
