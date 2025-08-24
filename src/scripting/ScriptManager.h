//
// Created by gabe on 6/23/25.
//

#pragma once
#include <sol/sol.hpp>
#include "core/module/Module.h"
#include "core/Entity.h"

namespace Engine {


	class ScriptManager : public Module {
	  public:
		void                      onInit() override;
		void                      onUpdate(float dt) override;
		void                      onGameStart() override;
		void                      onShutdown() override;
		[[nodiscard]] std::string name() const override { return "ScriptModule"; }
		void                      ReloadEditorScript();


		struct CollisionEvent {
			Entity& a;
			Entity& b;
		};

		std::vector<CollisionEvent> pendingCollisions;
		std::mutex                  collisionMutex; // optional for future threading safety

		sol::state    lua;
		sol::function luaUpdate;
	};
} // namespace Engine