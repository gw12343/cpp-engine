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
#include "components/impl/EntityMetadataComponent.h"
namespace Engine {

	// Module overrides
	void SceneManager::onInit()
	{
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
	void SceneManager::UpdateTransforms()
	{
		auto view = GetCurrentSceneRegistry().view<Components::EntityMetadata, Components::Transform>();
		for (auto [entity, metadata, transform] : view.each()) {
			Entity e{entity, GetCurrentScene()};


			if (!metadata.parentEntity.IsValid()) {
				UpdateTransformRecursive(e, glm::mat4(1.0f));
			}
		}
	}
	void SceneManager::UpdateTransformRecursive(Entity entity, const glm::mat4& parentMatrix)
	{
		auto& transform = entity.GetComponent<Components::Transform>();

		glm::mat4 localMatrix = glm::translate(glm::mat4(1.0f), transform.localPosition) * glm::mat4_cast(transform.localRotation) * glm::scale(glm::mat4(1.0f), transform.localScale);

		transform.worldMatrix   = parentMatrix * localMatrix;
		transform.worldPosition = glm::vec3(transform.worldMatrix[3]);
		transform.worldRotation = glm::quat_cast(transform.worldMatrix);
		transform.worldScale    = glm::vec3(glm::length(glm::vec3(transform.worldMatrix[0])), glm::length(glm::vec3(transform.worldMatrix[1])), glm::length(glm::vec3(transform.worldMatrix[2])));

		// Update children
		auto& hierarchy = entity.GetComponent<Components::EntityMetadata>();
		for (auto& childHandle : hierarchy.children) {
			auto childEntity = GetCurrentScene()->Get(childHandle);
			if (childEntity) {
				UpdateTransformRecursive(childEntity, transform.worldMatrix);
			}
		}
	}
} // namespace Engine

#include "assets/AssetManager.inl"
