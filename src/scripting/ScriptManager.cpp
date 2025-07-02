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
			lua.set_function("createEntity", [](const std::string& name) { return Engine::Entity::Create(name); });


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

	void ScriptManager::onUpdate(float dt)
	{
		// Editor script
		if (luaUpdate.valid()) {
			try {
				luaUpdate(dt);
			}
			catch (const sol::error& e) {
				log->error("Lua error in onUpdate: {}", e.what());
			}
		}

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