//
// Created by gabe on 6/24/25.
//
#include "components/Components.h"

#include "core/Engine.h"
#include "core/Entity.h"
#include "utils/Utils.h"
#include "imgui.h"
#include "ozz/animation/runtime/track.h"
#include <string>
#include "rendering/particles/ParticleManager.h"
#include "animation/AnimationManager.h"
#include "scripting/ScriptManager.h"

namespace Engine::Components {
	void LuaScript::OnAdded(Entity& entity)
	{
		env = sol::environment(GetScriptManager().lua, sol::create, GetScriptManager().lua.globals());

		env["gameObject"] = entity;

		try {
			// Load and execute the script
			GetScriptManager().lua.script_file(scriptPath, env);

			// Bind lifecycle functions if they exist
			start    = env["Start"];
			update   = env["Update"];
			shutdown = env["Shutdown"];

			if (start.valid()) {
				start(); // Optionally call Start() immediately
			}
		}
		catch (const sol::error& err) {
			std::cerr << "[LuaScript] Error in " << scriptPath << ": " << err.what() << std::endl;
		}
	}

	void LuaScript::RenderInspector(Engine::Entity& entity)
	{
		ImGui::Text("Path %s", scriptPath.c_str());
	}
} // namespace Engine::Components