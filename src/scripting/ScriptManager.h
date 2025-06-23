//
// Created by gabe on 6/23/25.
//

#pragma once
#include <sol/sol.hpp>
#include "core/module/Module.h"

namespace Engine {
	class ScriptManager : public Module {
	  public:
		void                      onInit() override;
		void                      onUpdate(float dt) override;
		void                      onShutdown() override;
		[[nodiscard]] std::string name() const override { return "ScriptModule"; }


		sol::state    lua;
		sol::function luaUpdate;
	};
} // namespace Engine