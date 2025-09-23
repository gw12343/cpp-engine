#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <entt/entt.hpp>
#include <map>
#include <vector>
#include "EntityHandle.h"

namespace Engine {
	class Entity; // forward declaration

	class Scene {
	  public:
		Scene(std::string name);
		Scene(std::string name, std::vector<Entity> entities);

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
