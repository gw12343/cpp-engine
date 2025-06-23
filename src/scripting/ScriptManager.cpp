//
// Created by gabe on 6/23/25.
//

#include "ScriptManager.h"
#include "glm/vec3.hpp"
#include "core/Entity.h"
#include "utils/ModelLoader.h"
#include "physics/PhysicsManager.h"
#include "core/Input.h"
namespace Engine {

	void ScriptManager::onInit()
	{
		log->info("Initializing Lua scripting...");
		lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::table, sol::lib::os);

		env[sol::metatable_key] = lua.create_table_with("__index", [](sol::this_state ts, const std::string& key) {
			std::cerr << "[Lua] WARNING: Tried to access undefined key: " << key << "\n";
			return sol::lua_nil;
		});

		try {
			lua.script_file("scripts/init.lua");

			lua["Input"] = lua.create_table();

			// Keyboard
			lua["Input"]["isKeyPressed"]          = &Input::IsKeyPressed;
			lua["Input"]["isKeyReleased"]         = &Input::IsKeyReleased;
			lua["Input"]["isKeyPressedThisFrame"] = &Input::IsKeyPressedThisFrame;

			// Mouse
			lua["Input"]["isMousePressed"] = &Engine::Input::IsMousePressed;
			lua["Input"]["getMouseDelta"]  = &Engine::Input::GetMouseDelta;
			lua["Input"]["getCursorMode"]  = &Engine::Input::GetCursorMode;
			lua["Input"]["setCursorMode"]  = &Engine::Input::SetCursorMode;

			// glm::vec2 binding if not already done
			lua.new_usertype<glm::vec2>("vec2", sol::constructors<glm::vec2(), glm::vec2(float, float)>(), "x", &glm::vec2::x, "y", &glm::vec2::y);

			// Optional: add functions with vec2 parameters (needs conversion)
			lua["Input"]["getMousePosition"] = &Engine::Input::GetMousePosition;
			lua["Input"]["setMousePosition"] = &Engine::Input::SetMousePosition;


			// Bind glm::vec3
			lua.new_usertype<glm::vec3>("vec3", sol::constructors<glm::vec3(), glm::vec3(float, float, float)>(), "x", &glm::vec3::x, "y", &glm::vec3::y, "z", &glm::vec3::z);

			// Bind Entity
			lua.new_usertype<Engine::Entity>("Entity", "getName", &Engine::Entity::GetName, "setName", &Engine::Entity::SetName);

			// create_entity(name)
			lua.set_function("create_entity", [](const std::string& name) {
				SPDLOG_ERROR("CALLING CREATE ENTITY========================");
				return Engine::Entity::Create(name);
			});

			lua.set_function("add_transform", [](Engine::Entity e, sol::table pos, sol::table rot, sol::table scale) {
				if (!e) return;

				auto toVec3 = [](sol::table t) { return glm::vec3(t.get_or("x", 0.0f), t.get_or("y", 0.0f), t.get_or("z", 0.0f)); };

				glm::vec3 p = toVec3(pos);
				glm::vec3 r = toVec3(rot);
				glm::vec3 s = toVec3(scale);

				e.AddComponent<Components::Transform>(p, r, s);
			});

			// add_model_renderer(entity, path)
			lua.set_function("add_model_renderer", [](Engine::Entity e, const std::string& path) {
				if (e) {
					auto model = Engine::Rendering::ModelLoader::LoadModel(path);
					e.AddComponent<Components::ModelRenderer>(model);
				}
			});

			// add_rigidbody(entity, id)
			lua.set_function("add_rigidbody", [](Engine::Entity e, int id) {
				if (e) e.AddComponent<Components::RigidBodyComponent>(GetPhysics().GetPhysicsSystem().get(), (BodyID) id);
			});


			if (lua["onInit"].valid()) {
				lua["onInit"]();
			}

			if (lua["onUpdate"].valid()) {
				luaUpdate = lua["onUpdate"];
			}
			else {
				log->warn("No onUpdate function found in Lua.");
			}
		}
		catch (const sol::error& e) {
			log->error("Lua error during init: {}", e.what());
		}
	}

	void ScriptManager::onUpdate(float dt)
	{
		if (luaUpdate.valid()) {
			try {
				luaUpdate(dt);
			}
			catch (const sol::error& e) {
				log->error("Lua error in onUpdate: {}", e.what());
			}
		}
	}

	void ScriptManager::onShutdown()
	{
		log->info("Shutting down Lua scripting...");
		if (lua["onShutdown"].valid()) {
			try {
				lua["onShutdown"]();
			}
			catch (const sol::error& e) {
				log->error("Lua error in onShutdown: {}", e.what());
			}
		}
	}
} // namespace Engine