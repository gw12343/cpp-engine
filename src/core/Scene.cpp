//
// Created by gabe on 8/17/25.
//

#include "core/Scene.h"
#include "Entity.h"

namespace Engine {
	Scene::Scene(std::string name) : m_name(std::move(name))
	{
		m_registry = std::make_shared<entt::registry>();
	}

	Scene::Scene(std::string name, std::vector<Entity> entities) : m_name(std::move(name))
	{
		m_registry   = std::make_shared<entt::registry>();
		m_entityList = std::move(entities);
	}

	Entity Scene::Get(const EntityHandle& handle)
	{
		auto it = m_entityMap.find(handle);
		if (it != m_entityMap.end()) return (it->second); // dereference pointer to return Entity
		return Entity{};                                  // invalid entity
	}

} // namespace Engine
