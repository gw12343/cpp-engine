//
// Created by gabe on 8/14/25.
//

#pragma once

#include "core/module/Module.h"
#include "entt/entt.hpp"

namespace Engine {

	class Entity;
	class SerializationManager : public Module {
		void onInit() override;
		void onUpdate(float dt) override;
		void onShutdown() override;

		[[nodiscard]] std::string name() const override { return "SerializationModule"; };

	  public:
		void SaveScene(const entt::registry& registry, const std::string& filename);

		std::vector<Entity> LoadScene(entt::registry& registry, const std::string& filename);
	};
} // namespace Engine
