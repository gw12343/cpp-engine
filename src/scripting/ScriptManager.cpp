//
// Created by gabe on 6/23/25.
//

#include "ScriptManager.h"
#include "glm/vec3.hpp"
#include "core/Entity.h"
#include "utils/ModelLoader.h"
#include "physics/PhysicsManager.h"


#define COMPONENT_METHODS(COMPONENT_TYPE, COMPONENT_NAME)                                                                                                                                                                                      \
	"Add" #COMPONENT_NAME, [](Entity& e) -> COMPONENT_TYPE& { return e.AddComponent<COMPONENT_TYPE>(); }, "Get" #COMPONENT_NAME, [](Entity& e) -> COMPONENT_TYPE& { return e.GetComponent<COMPONENT_TYPE>(); }, "Has" #COMPONENT_NAME,         \
	    [](Entity& e) -> bool { return e.HasComponent<COMPONENT_TYPE>(); }, "Remove" #COMPONENT_NAME, [](Entity& e) { e.RemoveComponent<COMPONENT_TYPE>(); }


namespace Engine {

	void ScriptManager::onInit()
	{
		log->info("Initializing Lua scripting...");
		lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::table, sol::lib::os);

		//		env[sol::metatable_key] = lua.create_table_with("__index", [](sol::this_state ts, const std::string& key) {
		//			std::cerr << "[Lua] WARNING: Tried to access undefined key: " << key << "\n";
		//			return sol::lua_nil;
		//		});

		try {
			lua.script_file("scripts/init.lua");


			// Bind glm::vec3
			// lua.new_usertype<glm::vec3>("vec3", sol::constructors<glm::vec3(), glm::vec3(float, float, float)>(), "x", &glm::vec3::x, "y", &glm::vec3::y, "z", &glm::vec3::z);

			// Bind Entity
			lua.new_usertype<Engine::Entity>("Entity",
			                                 "getName",
			                                 &Engine::Entity::GetName,
			                                 "setName",
			                                 &Engine::Entity::SetName,
			                                 COMPONENT_METHODS(Components::LuaScript, LuaScript),
			                                 COMPONENT_METHODS(Components::ShadowCaster, ShadowCaster),
			                                 COMPONENT_METHODS(Components::EntityMetadata, EntityMetadata),
			                                 COMPONENT_METHODS(Components::Transform, Transform),
			                                 COMPONENT_METHODS(Components::ModelRenderer, ModelRenderer),
			                                 COMPONENT_METHODS(Components::RigidBodyComponent, RigidBodyComponent),
			                                 COMPONENT_METHODS(Components::AudioSource, AudioSource),
			                                 COMPONENT_METHODS(Components::SkeletonComponent, SkeletonComponent),
			                                 COMPONENT_METHODS(Components::AnimationComponent, AnimationComponent),
			                                 COMPONENT_METHODS(Components::AnimationPoseComponent, AnimationPoseComponent),
			                                 COMPONENT_METHODS(Components::AnimationWorkerComponent, AnimationWorkerComponent),
			                                 COMPONENT_METHODS(Components::SkinnedMeshComponent, SkinnedMeshComponent),
			                                 COMPONENT_METHODS(Components::ParticleSystem, ParticleSystem));


			// create_entity(name)
			//			lua.set_function("create_entity", [](const std::string& name) {
			//				SPDLOG_ERROR("CALLING CREATE ENTITY========================");
			//				return Engine::Entity::Create(name);
			//			});

			//			lua.set_function("add_transform", [](Engine::Entity e, sol::table pos, sol::table rot, sol::table scale) {
			//				if (!e) return;
			//
			//				auto toVec3 = [](sol::table t) { return glm::vec3(t.get_or("x", 0.0f), t.get_or("y", 0.0f), t.get_or("z", 0.0f)); };
			//
			//				glm::vec3 p = toVec3(pos);
			//				glm::vec3 r = toVec3(rot);
			//				glm::vec3 s = toVec3(scale);
			//
			//				e.AddComponent<Components::Transform>(p, r, s);
			//			});


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

		GetRegistry().view<Components::LuaScript>().each([](entt::entity entity, Components::LuaScript& script) {
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