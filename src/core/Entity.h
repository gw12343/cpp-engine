#pragma once

#include "components/Components.h"
#include "EngineData.h"

#include <entt/entt.hpp>
#include <string>

namespace Engine {


	// Entity wrapper class for easier entity manipulation
	class Entity {
	  public:
		Entity() = default;
		Entity(entt::entity handle) : m_handle(handle) {}

		// Static methods for entity creation and destruction
		static Entity Create(const std::string& name = "");
		static void   Destroy(Entity entity);

		// Check if entity is valid
		explicit operator bool() const { return m_handle != entt::null; }

		// Comparison operators
		bool operator==(const Entity& other) const { return m_handle == other.m_handle; }
		bool operator!=(const Entity& other) const { return !(*this == other); }

		// Get the underlying entt handle
		[[nodiscard]] entt::entity GetHandle() const { return m_handle; }

		// Template method declarations
		template <typename T, typename... Args>
		T& AddComponent(Args&&... args)
		{
			T& component = GetRegistry().template emplace<T>(m_handle, std::forward<Args>(args)...);
			component.OnAdded(*this);
			return component;
		}


		template <typename T>
		T& GetComponent()
		{
			return GetRegistry().template get<T>(m_handle);
		}

		template <typename T>
		bool HasComponent() const
		{
			return GetRegistry().template all_of<T>(m_handle);
		}

		template <typename T>
		void RemoveComponent()
		{
			GetRegistry().template remove<T>(m_handle);
		}

		// Entity metadata helpers
		[[nodiscard]] const std::string& GetName() const;
		void                             SetName(const std::string& name);

		[[maybe_unused]] [[nodiscard]] const std::string& GetTag() const;
		[[maybe_unused]] void                             SetTag(const std::string& tag);

		[[nodiscard]] bool    IsActive() const;
		[[maybe_unused]] void SetActive(bool active);


	  private:
		entt::entity m_handle{entt::null};
	};
} // namespace Engine
