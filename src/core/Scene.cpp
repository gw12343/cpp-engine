//
// Created by gabe on 8/17/25.
//

#include "core/Scene.h"


namespace Engine {
	Scene::Scene(std::string name) : m_name(std::move(name))
	{
		m_registry = std::make_shared<entt::registry>();
	}

	Scene::Scene(std::string name, std::vector<Entity> entities) : m_name(std::move(name))
	{
		m_registry   = std::make_shared<entt::registry>();
		m_entityList = entities;
	}
} // namespace Engine