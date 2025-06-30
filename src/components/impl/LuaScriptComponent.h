//
// Created by gabe on 6/29/25.
//

#ifndef CPP_ENGINE_LUASCRIPTCOMPONENT_H
#define CPP_ENGINE_LUASCRIPTCOMPONENT_H

#include "components/Components.h"

namespace Engine::Components {
	class LuaScript : public Component {
	  public:
		LuaScript() = default;
		explicit LuaScript(std::string path) : scriptPath(path) {}

		virtual void OnAdded(Entity& entity) override;
		virtual void RenderInspector(Entity& entity) override;
		void         LoadScript(Engine::Entity& entity, std::string path);
		void         OnCollisionEnter(Entity& other);
		static void  AddBindings();


		std::string      scriptPath;
		sol::environment env;

		sol::function start;
		sol::function update;
		sol::function collisionEnter;
	};
} // namespace Engine::Components

#endif // CPP_ENGINE_LUASCRIPTCOMPONENT_H
