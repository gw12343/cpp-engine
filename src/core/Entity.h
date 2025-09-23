#pragma once

#include "components/Components.h"
#include "EngineData.h"
#include "Scene.h" // full definition needed here

#include <entt/entt.hpp>
#include <string>

namespace Engine {
	class Entity {
	  public:
		Entity() = default;
		Entity(entt::entity handle, Scene* scene) : m_handle(handle), m_scene(scene) {}

		static Entity Create(const std::string& name, Scene* scene);
		void          Destroy();
		void          MarkForDestruction();

		explicit operator bool() const { return m_handle != entt::null; }
		bool IsValid();

		bool operator==(const Entity& other) const { return m_handle == other.m_handle; }
		bool operator!=(const Entity& other) const { return !(*this == other); }

		[[nodiscard]] entt::entity GetHandle() const { return m_handle; }

		template <typename T, typename... Args>
		T& AddComponent(Args&&... args)
		{
			T& component = m_scene->GetRegistry()->template emplace<T>(m_handle, std::forward<Args>(args)...);
			component.OnAdded(*this);
			return component;
		}

		template <typename T>
		T& GetComponent()
		{
			return m_scene->GetRegistry()->template get<T>(m_handle);
		}

		template <typename T>
		bool HasComponent() const
		{
			return m_scene->GetRegistry()->template all_of<T>(m_handle);
		}

		template <typename T>
		void RemoveComponent()
		{
			m_scene->GetRegistry()->template remove<T>(m_handle);
		}

		[[nodiscard]] const std::string& GetName() const;
		void                             SetName(const std::string& name);

		[[maybe_unused]] [[nodiscard]] const std::string& GetTag() const;
		[[maybe_unused]] void                             SetTag(const std::string& tag);

		[[nodiscard]] bool    IsActive() const;
		[[maybe_unused]] void SetActive(bool active);

		Scene* m_scene{};

	  private:
		entt::entity m_handle{entt::null};
	};
} // namespace Engine
