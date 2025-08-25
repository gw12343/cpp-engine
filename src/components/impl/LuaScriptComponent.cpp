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

#include "misc/cpp/imgui_stdlib.h"

namespace Engine::Components {
	void LuaScript::OnRemoved(Entity& entity)
	{
		if (start.valid()) {
			start = sol::lua_nil;
		}
		if (update.valid()) {
			update = sol::lua_nil;
		}
		if (collisionEnter.valid()) {
			collisionEnter = sol::lua_nil;
		}
		if (playerCollisionEnter.valid()) {
			playerCollisionEnter = sol::lua_nil;
		}

		if (env.valid()) {
			env.clear();
			env = sol::lua_nil;
		}

		GetScriptManager().lua.collect_garbage();
	}


	void LuaScript::OnAdded(Entity& entity)
	{
		LoadScript(entity, scriptPath);
		SyncToLua();
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

	void LuaScript::OnPlayerCollisionEnter()
	{
		if (playerCollisionEnter.valid()) {
			try {
				playerCollisionEnter();
			}
			catch (const sol::error& err) {
				GetScriptManager().log->error("[LuaScript] PlayerCollisionEnter error in {}: {}", scriptPath, err.what());
			}
		}
	}

	void LuaScript::RenderInspector(Engine::Entity& entity)
	{
		if (ImGui::InputText("Script Path", &scriptPath)) {
			GetScriptManager().log->info("Reloading script.");
			OnRemoved(entity);
			LoadScript(entity, scriptPath);
		}

		if (env.valid() && variables.valid()) {
			for (auto& kv : variables) {
				std::string key = kv.first.as<std::string>();

				if (kv.second.is<double>()) {
					auto value = (float) kv.second.as<double>();
					if (ImGui::DragFloat(key.c_str(), &value)) {
						variables[key] = value;
						SyncFromLua();
					}
				}
				else if (kv.second.is<std::string>()) {
					std::string value = kv.second.as<std::string>();
					if (ImGui::InputText(key.c_str(), &value)) {
						variables[key] = value;
						SyncFromLua();
					}
				}
			}
		}
	}

	void LuaScript::SyncFromLua()
	{
		GetScriptManager().log->info("loading var values from lua");
		cppVariables.clear();
		if (variables.valid()) {
			for (auto& kv : variables) {
				auto key = kv.first.as<std::string>();
				if (kv.second.is<double>())
					cppVariables[key] = static_cast<float>(kv.second.as<double>());
				else if (kv.second.is<std::string>())
					cppVariables[key] = kv.second.as<std::string>();
			}
		}
	}

	void LuaScript::SyncToLua()
	{
		if (!variables.valid()) return;

		for (auto& [key, val] : cppVariables) {
			sol::object existing = variables[key];

			// Only update if the key already exists in the Lua table
			if (existing.valid()) {
				if (std::holds_alternative<float>(val))
					variables[key] = std::get<float>(val);
				else if (std::holds_alternative<std::string>(val))
					variables[key] = std::get<std::string>(val);
			}
		}
	}

	void LuaScript::LoadScript(Engine::Entity& entity, std::string path)
	{
		this->scriptPath = std::move(path);

		// Clear old Lua environment and bound functions in case we're reloading
		env                  = sol::environment();
		start                = sol::function();
		update               = sol::function();
		collisionEnter       = sol::function();
		playerCollisionEnter = sol::function(); // NEW
		variables            = sol::table();    // NEW

		if (scriptPath.empty()) return;

		// Create a fresh Lua environment
		env = sol::environment(GetScriptManager().lua, sol::create, GetScriptManager().lua.globals());

		// Inject gameObject
		env["gameObject"] = entity;

		try {
			// Load the script into the environment
			GetScriptManager().lua.script_file(scriptPath, env);

			// Bind lifecycle functions
			start                = env["Start"];
			update               = env["Update"];
			collisionEnter       = env["CollisionEnter"];
			playerCollisionEnter = env["PlayerCollisionEnter"];
			sol::object vars     = env["variables"];

			if (vars.is<sol::table>()) {
				variables = vars.as<sol::table>();
				// Set any saved values
				SyncToLua();
				// Load any default non overridden values from the script
				SyncFromLua();
			}
			else {
				// Create an empty table so editor/serialization logic still works
				variables = GetScriptManager().lua.create_table();
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