//
// Created by gabe on 6/23/25.
//

#include "ScriptManager.h"

#include "core/Entity.h"
#include "components/impl/AnimationComponent.h"
#include "assets/impl/ModelLoader.h"
#include "physics/PhysicsManager.h"
#include "components/impl/LuaScriptComponent.h"
#include "components/impl/ShadowCasterComponent.h"
#include "components/impl/EntityMetadataComponent.h"
#include "components/impl/ModelRendererComponent.h"
#include "components/impl/RigidBodyComponent.h"
#include "components/impl/AudioSourceComponent.h"
#include "components/impl/SkeletonComponent.h"
#include "components/impl/AnimationPoseComponent.h"
#include "components/impl/AnimationWorkerComponent.h"
#include "components/impl/SkinnedMeshComponent.h"
#include "components/impl/ParticleSystemComponent.h"
#include "components/AllComponents.h"

#include "physics/PhysicsManager.h"

#define COMPONENT_METHODS(COMPONENT_TYPE, COMPONENT_NAME)                                                                                                                                                                                      \
	"Add" #COMPONENT_NAME, [](Entity& e) -> COMPONENT_TYPE& { return e.AddComponent<COMPONENT_TYPE>(); }, "Get" #COMPONENT_NAME, [](Entity& e) -> COMPONENT_TYPE& { return e.GetComponent<COMPONENT_TYPE>(); }, "Has" #COMPONENT_NAME,         \
	    [](Entity& e) -> bool { return e.HasComponent<COMPONENT_TYPE>(); }, "Remove" #COMPONENT_NAME, [](Entity& e) { e.RemoveComponent<COMPONENT_TYPE>(); }


namespace Engine {

	void ScriptManager::onInit()
	{
		log->info("Initializing Lua scripting...");
		lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::table, sol::lib::os);


		try {
			lua.script_file("scripts/init.lua");

#define X(type, name, fancy) COMPONENT_METHODS(type, name),
			// Bind Entity
			lua.new_usertype<Engine::Entity>("Entity",
			                                 "getName",
			                                 &Engine::Entity::GetName,
			                                 "setName",
			                                 &Engine::Entity::SetName,

			                                 COMPONENT_LIST COMPONENT_METHODS(Components::EntityMetadata, EntityMetadata));
#undef X


			// create_entity(name)
			lua.set_function("createEntity", [](const std::string& name) { return Engine::Entity::Create(name, GetCurrentScene()); });


			lua.set_function("print", [](sol::variadic_args va) {
				std::string out;
				for (auto v : va) {
					out += v.get<std::string>() + " ";
				}
				if (!out.empty()) out.pop_back(); // remove trailing space

				Logger::get("script")->debug("[Lua] {}", out);
			});

			if (lua["EditorInit"].valid()) {
				lua["EditorInit"]();
			}

			if (lua["EditorUpdate"].valid()) {
				luaUpdate = lua["EditorUpdate"];
			}
			else {
				log->warn("No onUpdate function found in Lua.");
			}
		}
		catch (const sol::error& e) {
			log->error("Lua error during init: {}", e.what());
		}
	}


	float scriptDeltaTime = 0.0;

	void ScriptManager::onUpdate(float dt)
	{
		scriptDeltaTime = dt;
		// Editor script
		if (luaUpdate.valid()) {
			try {
				luaUpdate(dt);
			}
			catch (const sol::error& e) {
				log->error("Lua error in onUpdate: {}", e.what());
			}
		}

		if (GetState() != PLAYING) return;


		{
			std::lock_guard<std::mutex> lock(collisionMutex);

			for (const auto& event : pendingCollisions) {
				Entity& a = event.a;
				Entity& b = event.b;


				if (a.HasComponent<Components::LuaScript>()) {
					auto& sc = a.GetComponent<Components::LuaScript>();
					sc.OnCollisionEnter(b);
				}

				if (b.HasComponent<Components::LuaScript>()) {
					auto& sc = b.GetComponent<Components::LuaScript>();
					sc.OnCollisionEnter(a);
				}
			}

			pendingCollisions.clear();
		}


		// User scripts
		GetCurrentSceneRegistry().view<Components::LuaScript>().each([](entt::entity entity, Components::LuaScript& script) {
			if (script.env) {
				script.env["deltaTime"] = scriptDeltaTime;
			}
			if (script.update.valid()) {
				sol::protected_function_result result = script.update();
				if (!result.valid()) {
					sol::error err = result;
					std::cerr << "Lua Update() error for entity " << int(entity) << ": " << err.what() << std::endl;
				}
			}
		});
	}

	void ScriptManager::onShutdown()
	{
		log->info("Shutting down Lua scripting...");
		if (lua["EditorShutdown"].valid()) {
			try {
				lua["EditorShutdown"]();
			}
			catch (const sol::error& e) {
				log->error("Lua error in onShutdown: {}", e.what());
			}
		}
	}
} // namespace Engine