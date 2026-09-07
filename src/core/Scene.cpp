//
// Created by gabe on 8/17/25.
//

#include "core/Entity.h"
#include "core/Scene.h"

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

	Scene::~Scene()                         = default;
	Scene::Scene(Scene&&) noexcept          = default;
	Scene& Scene::operator=(Scene&&) noexcept = default;

	Entity Scene::Get(const EntityHandle& handle)
	{
		if (!handle.IsValid()) {
			return {};
		}
		auto it = m_entityMap.find(handle);
		if (it == m_entityMap.end()) {
			return {};
		}
		if (!it->second.IsValid()) {
			return {};
		}
		return it->second;
	}

} // namespace Engine
