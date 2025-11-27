#pragma once
#include <sol/sol.hpp>
#include "core/module/Module.h"
#include "core/Entity.h"
#include "EventBus.h"

namespace Engine {

	class ScriptManager : public Module {
	  public:
		void                      onInit() override;
		void                      onUpdate(float dt) override;
		void                      onGameStart() override;
		void                      onShutdown() override;
		[[nodiscard]] std::string name() const override { return "ScriptModule"; }
		void                      ReloadEditorScript();
		
		// Get event bus for external access
		EventBus& GetEventBus() { return eventBus; }


		struct CollisionEvent {
			Entity& a;
			Entity& b;
		};

		std::vector<CollisionEvent> pendingCollisions;
		std::vector<Entity>         pendingCharacterCollisions;
		std::mutex                  collisionMutex; // optional for future threading safety

		sol::state    lua;
		sol::function luaUpdate;
		
		// Event bus for publish/subscribe pattern
		EventBus      eventBus;
	};
} // namespace Engine