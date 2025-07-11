//
// Created by gabe on 6/24/25.
//
#include "components/Components.h"
#include "components/impl/LuaScriptComponent.h"

#include "core/Engine.h"
#include "core/Entity.h"
#include "utils/Utils.h"
#include "imgui.h"
#include "ozz/animation/runtime/track.h"
#include <string>
#include <utility>
#include "rendering/particles/ParticleManager.h"
#include "animation/AnimationManager.h"
#include "scripting/ScriptManager.h"


namespace Engine::Components {
	void LuaScript::OnAdded(Entity& entity)
	{
		LoadScript(entity, scriptPath);
	}


	void LuaScript::OnCollisionEnter(Entity& other)
	{
		if (collisionEnter.valid()) {
			try {
				collisionEnter(other);
			}
			catch (const sol::error& err) {
				GetScriptManager().log->error("[LuaScript] CollisionEnter error in {}: {}", scriptPath, err.what());
			}
		}
	}

	void LuaScript::RenderInspector(Engine::Entity& entity)
	{
		ImGui::Text("Path %s", scriptPath.c_str());
	}

	void LuaScript::LoadScript(Engine::Entity& entity, std::string path)
	{
		this->scriptPath = std::move(path);

		// Clear old Lua environment and bound functions in case we're reloading
		env            = sol::environment();
		start          = sol::function();
		update         = sol::function();
		collisionEnter = sol::function(); // NEW

		if (scriptPath.empty()) return;

		// Create a fresh Lua environment
		env = sol::environment(GetScriptManager().lua, sol::create, GetScriptManager().lua.globals());

		// Inject gameObject
		env["gameObject"] = entity;

		try {
			// Load the script into the environment
			GetScriptManager().lua.script_file(scriptPath, env);

			// Bind lifecycle functions
			start          = env["Start"];
			update         = env["Update"];
			collisionEnter = env["CollisionEnter"];

			if (start.valid()) {
				start(); // Optionally run
			}
		}
		catch (const sol::error& err) {
			GetScriptManager().log->error("[LuaScript] Error in {}: {}", scriptPath, err.what());
		}
	}

	void LuaScript::AddBindings()
	{
		auto& lua = GetScriptManager().lua;

		lua.new_usertype<LuaScript>("LuaScript",
		                            "scriptPath",
		                            &LuaScript::scriptPath,

		                            "setScript",
		                            [](LuaScript& self, Entity entity, const std::string& path) {
			                            if (!entity.HasComponent<LuaScript>()) {
				                            GetScriptManager().log->error("Entity doesn't have a LuaScript component!");
				                            return;
			                            }
			                            auto& script = entity.GetComponent<LuaScript>();
			                            script.LoadScript(entity, path);
			                            // TODO give {@code LuaScript} a refrence to its entity??
		                            });
	}
} // namespace Engine::Components