#pragma once


#include "Scene.h"

namespace Engine {
	// Template method declarations
	template <typename T, typename... Args>
	T& Entity::AddComponent(Args&&... args)
	{
		T& component = m_scene->GetRegistry()->template emplace<T>(m_handle, std::forward<Args>(args)...);
		component.OnAdded(*this);
		return component;
	}


	template <typename T>
	T& Entity::GetComponent()
	{
		return m_scene->GetRegistry()->template get<T>(m_handle);
	}

	template <typename T>
	[[nodiscard]] bool Entity::HasComponent() const
	{
		return m_scene->GetRegistry()->template all_of<T>(m_handle);
	}

	template <typename T>
	void Entity::RemoveComponent()
	{
		m_scene->GetRegistry()->template remove<T>(m_handle);
	}

}
