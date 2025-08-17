//
// Created by gabe on 8/16/25.
//

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <entt/entt.hpp>
#include <spdlog/spdlog.h>


namespace Engine {

	// A single scene, essentially just a wrapper for entt::registry
	class Scene {
	  public:
		Scene(std::string name) : m_name(std::move(name)) { m_registry = std::make_shared<entt::registry>(); }

		std::shared_ptr<entt::registry> GetRegistry() { return m_registry; }

		const std::string& GetName() const { return m_name; }

	  private:
		std::string                     m_name;
		std::shared_ptr<entt::registry> m_registry;
	};
} // namespace Engine