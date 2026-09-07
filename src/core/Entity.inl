#pragma once

#include "Scene.h"
#include "utils/Logger.h"

namespace Engine {
	template <typename T, typename... Args>
	T& Entity::AddComponent(Args&&... args)
	{
		if (!IsValid()) {
			Logger::get("core")->error("AddComponent on invalid entity");
			static T dummy{};
			return dummy;
		}
		T& component = m_scene->GetRegistry()->template emplace<T>(m_handle, std::forward<Args>(args)...);
		component.OnAdded(*this);
		return component;
	}

	template <typename T>
	T* Entity::TryGetComponent()
	{
		if (!IsValid()) {
			return nullptr;
		}
		return m_scene->GetRegistry()->template try_get<T>(m_handle);
	}

	template <typename T>
	const T* Entity::TryGetComponent() const
	{
		if (!IsValid()) {
			return nullptr;
		}
		return m_scene->GetRegistry()->template try_get<T>(m_handle);
	}

	template <typename T>
	T& Entity::GetComponent()
	{
		if (T* p = TryGetComponent<T>()) {
			return *p;
		}
		Logger::get("core")->error("GetComponent on invalid entity or missing component");
		static T dummy{};
		return dummy;
	}

	template <typename T>
	const T& Entity::GetComponent() const
	{
		if (const T* p = TryGetComponent<T>()) {
			return *p;
		}
		Logger::get("core")->error("GetComponent on invalid entity or missing component");
		static T dummy{};
		return dummy;
	}

	template <typename T>
	[[nodiscard]] bool Entity::HasComponent() const
	{
		return TryGetComponent<T>() != nullptr;
	}

	template <typename T>
	void Entity::RemoveComponent()
	{
		T* live = TryGetComponent<T>();
		if (!live) {
			return;
		}
		live->OnRemoved(*this);
		if (T* still = TryGetComponent<T>()) {
			(void) still;
			m_scene->GetRegistry()->template remove<T>(m_handle);
		}
	}

} // namespace Engine
