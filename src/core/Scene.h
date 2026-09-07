//
// Created by gabe on 8/16/25.
//

#pragma once

#include "EntityHandle.h"

#include <entt/entt.hpp>

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace Engine {
	class Entity;

	class Scene {
	  public:
		Scene(std::string name);
		Scene(std::string name, std::vector<Entity> entities);
		~Scene();
		Scene(const Scene&)            = delete;
		Scene& operator=(const Scene&) = delete;
		Scene(Scene&&) noexcept;
		Scene& operator=(Scene&&) noexcept;

		std::shared_ptr<entt::registry> GetRegistry() { return m_registry; }

		const std::string& GetName() const { return m_name; }

		Entity Get(const EntityHandle& handle);

		std::vector<Entity>            m_entityList;
		std::map<EntityHandle, Entity> m_entityMap;

	  private:
		std::string                     m_name;
		std::shared_ptr<entt::registry> m_registry;
	};
} // namespace Engine
