#pragma once

#include "components/Components.h"

#include <entt/entt.hpp>
#include <string>

namespace Engine {

	// Forward declaration of Engine class
	class GEngine;

	// Entity wrapper class for easier entity manipulation
	class Entity {
	  public:
		Entity() = default;
		Entity(entt::entity handle, GEngine* engine) : m_handle(handle), m_engine(engine) {}

		// Static methods for entity creation and destruction
		static Entity Create(GEngine* engine, const std::string& name = "");
		static void   Destroy(Entity entity);

		// Check if entity is valid
		explicit operator bool() const { return m_handle != entt::null && m_engine != nullptr; }

		// Comparison operators
		bool operator==(const Entity& other) const { return m_handle == other.m_handle && m_engine == other.m_engine; }
		bool operator!=(const Entity& other) const { return !(*this == other); }

		// Get the underlying entt handle
		[[nodiscard]] entt::entity GetHandle() const { return m_handle; }

		// Template method declarations
		template <typename T, typename... Args>
		T& AddComponent(Args&&... args)
		{
			T& component = m_engine->GetRegistry().template emplace<T>(m_handle, std::forward<Args>(args)...);
			component.OnAdded(*this);
			return component;
		}

		template <typename T>
		T& GetComponent()
		{
			return m_engine->GetRegistry().template get<T>(m_handle);
		}

		template <typename T>
		bool HasComponent() const
		{
			return m_engine->GetRegistry().template all_of<T>(m_handle);
		}

		template <typename T>
		void RemoveComponent()
		{
			m_engine->GetRegistry().template remove<T>(m_handle);
		}

		// Entity metadata helpers
		[[nodiscard]] const std::string& GetName() const;
		void                             SetName(const std::string& name);

		[[maybe_unused]] [[nodiscard]] const std::string& GetTag() const;
		[[maybe_unused]] void                             SetTag(const std::string& tag);

		[[nodiscard]] bool    IsActive() const;
		[[maybe_unused]] void SetActive(bool active);

		GEngine* GetEngine() const { return m_engine; }

		GEngine* m_engine = nullptr;

	  private:
		entt::entity m_handle{entt::null};
	};
} // namespace Engine
