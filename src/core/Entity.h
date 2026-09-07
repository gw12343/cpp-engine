#pragma once

#include "EntityHandle.h"

#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <string>
#include <vector>

namespace Engine {
	class Scene;

	class Entity {
	  public:
		Entity() = default;
		Entity(entt::entity handle, Scene* scene) : m_handle(handle), m_scene(scene) {}

		static Entity Create(const std::string& name, Scene* scene);
		void          Destroy();
		void          MarkForDestruction();

		explicit operator bool() const { return IsValid(); }

		bool IsValid() const;

		bool operator==(const Entity& other) const { return m_handle == other.m_handle; }
		bool operator!=(const Entity& other) const { return !(*this == other); }

		[[nodiscard]] entt::entity GetENTTHandle() const { return m_handle; }

		EntityHandle GetEntityHandle();

		std::vector<EntityHandle> GetChildren();

		void SetParent(const EntityHandle& parent);

		void SetWorldTransform(glm::vec3 worldPosition, glm::quat worldRotation, glm::vec3 worldScale);

		template <typename T, typename... Args>
		T& AddComponent(Args&&... args);

		template <typename T>
		T& GetComponent();

		template <typename T>
		const T& GetComponent() const;

		template <typename T>
		T* TryGetComponent();

		template <typename T>
		const T* TryGetComponent() const;

		template <typename T>
		[[nodiscard]] bool HasComponent() const;

		template <typename T>
		void RemoveComponent();

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
