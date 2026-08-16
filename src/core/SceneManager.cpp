//
// Created by gabe on 8/16/25.
//

#include "SceneManager.h"

#include <utility>

#include "physics/PhysicsManager.h"
#include "scripting/ScriptManager.h"
#include "components/impl/RigidBodyComponent.h"

#include "components/impl/EntityMetadataComponent.h"
#include "core/ThreadPool.h"
#include "core/EngineData.h"
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

		auto   physicsView = GetCurrentSceneRegistry().view<Components::RigidBodyComponent>();
		Scene* s           = GetAssetManager().Get(m_activeScene);
		for (auto [entity, rb] : physicsView.each()) {
			physics.bodyToEntityMap[rb.bodyID] = Entity(entity, s);
		}
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
		for (auto& childHandle : hierarchy.children) {
			auto childEntity = GetCurrentScene()->Get(childHandle);
			if (childEntity) {
				UpdateTransformRecursive(childEntity, transform.GetWorldMatrix(), true);
			}
		}
	}
} // namespace Engine

#include "assets/AssetManager.inl"
