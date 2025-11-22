#pragma once

#include "components/Components.h"
#include "EngineData.h"
#include "Scene.h"

#include <entt/entt.hpp>
#include <string>

namespace Engine {
	class EntityHandle;

	// Entity wrapper class for easier entity manipulation
	class Entity {
	  public:
		Entity() = default;
		Entity(entt::entity handle, Scene* scene) : m_handle(handle), m_scene(scene) {}

		// Static methods for entity creation and destruction
		static Entity Create(const std::string& name, Scene* scene);
		void          Destroy();
		void          MarkForDestruction();

		// Check if entity is valid
		explicit operator bool() const { return m_handle != entt::null; }

		bool IsValid();

		// Comparison operators
		bool operator==(const Entity& other) const { return m_handle == other.m_handle; }
		bool operator!=(const Entity& other) const { return !(*this == other); }

		// Get the underlying entt handle
		[[nodiscard]] entt::entity GetENTTHandle() const { return m_handle; }

		EntityHandle GetEntityHandle();

		std::vector<EntityHandle> GetChildren();

		void SetParent(const EntityHandle& parent);

		void SetWorldTransform(glm::vec3 worldPosition, glm::quat worldRotation, glm::vec3 worldScale);


		// Template method declarations
		template <typename T, typename... Args>
		T& AddComponent(Args&&... args);


		template <typename T>
		T& GetComponent();

		template <typename T>
		[[nodiscard]] bool HasComponent() const;

		template <typename T>
		void RemoveComponent();

		// Entity metadata helpers
		[[nodiscard]] const std::string& GetName() const;
		void                             SetName(const std::string& name);

		[[maybe_unused]] [[nodiscard]] const std::string& GetTag() const;
		[[maybe_unused]] void                             SetTag(const std::string& tag);

		[[nodiscard]] bool    IsActive() const;
		[[maybe_unused]] void SetActive(bool active);


		Scene* m_scene;

	  private:
		entt::entity m_handle{entt::null};
		void         RemoveChild(const EntityHandle& handle);
	};
} // namespace Engine

#include "Entity.inl"