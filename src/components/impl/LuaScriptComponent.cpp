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
#include "rendering/ui/InspectorUI.h"


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
		if (LeftLabelInputText("Script Path", &scriptPath)) {
			GetScriptManager().log->info("Reloading script.");
			OnRemoved(entity);
			LoadScript(entity, scriptPath);
		}

		if (ImGui::TreeNode("Variables")) {
			if (env.valid() && variables.valid()) {
				for (auto& kv : variables) {
					std::string key = kv.first.as<std::string>();
					sol::object obj = kv.second;

					switch (obj.get_type()) {
						case sol::type::number: {
							auto value = static_cast<float>(obj.as<double>());
							if (LeftLabelDragFloat(key.c_str(), &value)) {
								variables[key]    = value;
								cppVariables[key] = value;
								SyncFromLua();
							}
							break;
						}
						case sol::type::string: {
							std::string value = obj.as<std::string>();
							if (LeftLabelInputText(key.c_str(), &value)) {
								variables[key]    = value;
								cppVariables[key] = value;
								SyncFromLua();
							}
							break;
						}
						case sol::type::boolean: {
							bool value = obj.as<bool>();
							if (LeftLabelCheckbox(key.c_str(), &value)) {
								variables[key]    = value;
								cppVariables[key] = value;
								SyncFromLua();
							}
							break;
						}
						case sol::type::userdata: {
#define ASSET_CHK(nme, typ)                                                                                                                                                                                                                    \
	else if (obj.is<AssetHandle<typ>>())                                                                                                                                                                                                       \
	{                                                                                                                                                                                                                                          \
		auto handle = obj.as<AssetHandle<typ>>();                                                                                                                                                                                              \
		if (LeftLabelAsset##nme(key.c_str(), &handle)) {                                                                                                                                                                                       \
			variables[key]    = handle;                                                                                                                                                                                                        \
			cppVariables[key] = handle;                                                                                                                                                                                                        \
			SyncFromLua();                                                                                                                                                                                                                     \
		}                                                                                                                                                                                                                                      \
	}


							// --- vec3 ---
							if (obj.is<glm::vec3>()) {
								glm::vec3 value        = obj.as<glm::vec3>();
								float     vec_array[3] = {value.x, value.y, value.z};
								if (LeftLabelDragFloat3(key.c_str(), vec_array)) {
									value             = {vec_array[0], vec_array[1], vec_array[2]};
									variables[key]    = value;
									cppVariables[key] = value;
									SyncFromLua();
								}
							}

							ASSET_CHK(Texture, Texture)
							ASSET_CHK(Model, Rendering::Model)
							ASSET_CHK(Material, Material)
							ASSET_CHK(Scene, Scene)
							ASSET_CHK(Terrain, Terrain::TerrainTile)
							ASSET_CHK(Particle, Particle)
							ASSET_CHK(Sound, Audio::SoundBuffer)
							else if (obj.is<EntityHandle>())
							{
								auto handle = obj.as<EntityHandle>();
								if (LeftLabelEntity(key.c_str(), &handle)) {
									variables[key]    = handle;
									cppVariables[key] = handle;
									SyncFromLua();
								}
							}
							else if (obj.is<std::vector<EntityHandle>>())
							{
								auto handle = obj.as<std::vector<EntityHandle>>();
								if (LeftLabelEntityVector(key.c_str(), handle)) {
									variables[key]    = handle;
									cppVariables[key] = handle;
									SyncFromLua();
								}
							}
							// TODO add asset vector types

							break;
						}
						default:
							ImGui::Text("%s (Unsupported Lua type)", key.c_str());
							break;
					}
				}
			}
			ImGui::TreePop();
		}
	}


	void LuaScript::SyncFromLua()
	{
		if (!variables.valid()) return;

		cppVariables.clear();

		for (auto& kv : variables) {
			std::string key = kv.first.as<std::string>();

			if (kv.second.is<double>()) {
				cppVariables[key] = static_cast<float>(kv.second.as<double>());
			}
			else if (kv.second.is<std::string>()) {
				cppVariables[key] = kv.second.as<std::string>();
			}
			else if (kv.second.is<glm::vec3>()) {
				cppVariables[key] = kv.second.as<glm::vec3>();
			}
			else if (kv.second.is<bool>()) {
				cppVariables[key] = kv.second.as<bool>();
			}
			else if (kv.second.is<int>()) {
				cppVariables[key] = kv.second.as<int>();
			}
			else if (kv.second.is<AssetHandle<Texture>>()) {
				cppVariables[key] = kv.second.as<AssetHandle<Texture>>();
			}
			else if (kv.second.is<AssetHandle<Rendering::Model>>()) {
				cppVariables[key] = kv.second.as<AssetHandle<Rendering::Model>>();
			}
			else if (kv.second.is<AssetHandle<Material>>()) {
				cppVariables[key] = kv.second.as<AssetHandle<Material>>();
			}
			else if (kv.second.is<AssetHandle<Scene>>()) {
				cppVariables[key] = kv.second.as<AssetHandle<Scene>>();
			}
			else if (kv.second.is<AssetHandle<Terrain::TerrainTile>>()) {
				cppVariables[key] = kv.second.as<AssetHandle<Terrain::TerrainTile>>();
			}
			else if (kv.second.is<AssetHandle<Particle>>()) {
				cppVariables[key] = kv.second.as<AssetHandle<Particle>>();
			}
			else if (kv.second.is<AssetHandle<Audio::SoundBuffer>>()) {
				cppVariables[key] = kv.second.as<AssetHandle<Audio::SoundBuffer>>();
			}
			else if (kv.second.is<EntityHandle>()) {
				cppVariables[key] = kv.second.as<EntityHandle>();
			}
			else if (kv.second.is<std::vector<EntityHandle>>()) {
				cppVariables[key] = kv.second.as<std::vector<EntityHandle>>();
			}
			else {
				SPDLOG_WARN("TRYING TO LOAD INVALID VAR VALUE FROM LUA");
			}
		}
	}

	void LuaScript::SyncToLua()
	{
		if (!variables.valid()) return;

		for (auto& [key, value] : cppVariables) {
			sol::object existing = variables[key];
			if (existing.valid()) {
				std::visit(
				    [&](auto&& arg) {
					    variables[key] = arg; // only overwrite if Lua already had the key
				    },
				    value);
			}
		}
	}

	Entity GetEntityFromHandle(const EntityHandle& handle)
	{
		Scene* s = GetCurrentScene();
		if (s->m_entityMap.count(handle)) {
			return s->m_entityMap[handle];
		}
		else {
			GetScriptManager().log->warn("Script requested an invalid entity: {}", handle.GetID());
			return {};
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
		playerCollisionEnter = sol::function();
		variables            = sol::table();

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

			env.set_function("getEntityFromHandle", GetEntityFromHandle);
		}
		catch (const sol::error& err) {
			GetScriptManager().log->error("[LuaScript] Error in {}: {}", scriptPath, err.what());
		}
	}

	void LuaScript::AddBindings()
	{
		auto& lua = GetScriptManager().lua;


		lua.new_usertype<LuaScript>(
		    "LuaScript",
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
		    },

		    // safe setter: only overwrites if already present
		    "setVariable",
		    [](LuaScript& self, const std::string& name, const sol::object& value) {
			    if (!self.variables.valid()) {
				    GetScriptManager().log->error("LuaScript has no variable table to set into!");
				    return;
			    }

			    sol::optional<sol::object> existing = self.variables[name];
			    if (!existing) {
				    GetScriptManager().log->warn("Variable '{}' does not exist in LuaScript, ignoring setVariable.", name);
				    return;
			    }

			    self.variables[name] = value;
		    },

		    // getter
		    "getVariable",
		    [](LuaScript& self, const std::string& name) -> sol::object {
			    if (!self.variables.valid()) {
				    GetScriptManager().log->error("LuaScript has no variable table to get from!");
				    return sol::nil;
			    }

			    sol::optional<sol::object> existing = self.variables[name];
			    if (!existing) {
				    GetScriptManager().log->warn("Variable '{}' does not exist in LuaScript.", name);
				    return sol::nil;
			    }

			    return self.variables[name];
		    });

		// Asset Handle bindings
		lua.new_usertype<AssetHandle<Texture>>("TextureHandle", "getGuid", &AssetHandle<Texture>::GetID, "isValid", &AssetHandle<Texture>::IsValid, "clear", [](AssetHandle<Texture>& self) { self = AssetHandle<Texture>(); });

		lua.new_usertype<AssetHandle<Rendering::Model>>(
		    "ModelHandle", "getGuid", &AssetHandle<Rendering::Model>::GetID, "isValid", &AssetHandle<Rendering::Model>::IsValid, "clear", [](AssetHandle<Rendering::Model>& self) { self = AssetHandle<Rendering::Model>(); });

		lua.new_usertype<AssetHandle<Material>>("MaterialHandle", "getGuid", &AssetHandle<Material>::GetID, "isValid", &AssetHandle<Material>::IsValid, "clear", [](AssetHandle<Material>& self) { self = AssetHandle<Material>(); });

		lua.new_usertype<AssetHandle<Scene>>("SceneHandle", "getGuid", &AssetHandle<Scene>::GetID, "isValid", &AssetHandle<Scene>::IsValid, "clear", [](AssetHandle<Scene>& self) { self = AssetHandle<Scene>(); });

		lua.new_usertype<AssetHandle<Terrain::TerrainTile>>(
		    "TerrainTileHandle", "getGuid", &AssetHandle<Terrain::TerrainTile>::GetID, "isValid", &AssetHandle<Terrain::TerrainTile>::IsValid, "clear", [](AssetHandle<Terrain::TerrainTile>& self) {
			    self = AssetHandle<Terrain::TerrainTile>();
		    });

		lua.new_usertype<AssetHandle<Particle>>("ParticleHandle", "getGuid", &AssetHandle<Particle>::GetID, "isValid", &AssetHandle<Particle>::IsValid, "clear", [](AssetHandle<Particle>& self) { self = AssetHandle<Particle>(); });

		lua.new_usertype<AssetHandle<Audio::SoundBuffer>>(
		    "SoundHandle", "getGuid", &AssetHandle<Audio::SoundBuffer>::GetID, "isValid", &AssetHandle<Audio::SoundBuffer>::IsValid, "clear", [](AssetHandle<Audio::SoundBuffer>& self) { self = AssetHandle<Audio::SoundBuffer>(); });

		// Factory functions for creating asset handles
		lua.set_function("tex", []() { return AssetHandle<Texture>(); });
		lua.set_function("model", []() { return AssetHandle<Rendering::Model>(); });
		lua.set_function("material", []() { return AssetHandle<Material>(); });
		lua.set_function("scene", []() { return AssetHandle<Scene>(); });
		lua.set_function("terrainTile", []() { return AssetHandle<Terrain::TerrainTile>(); });
		lua.set_function("particle", []() { return AssetHandle<Particle>(); });
		lua.set_function("sound", []() { return AssetHandle<Audio::SoundBuffer>(); });

		// Factory for entity handle
		lua.new_usertype<EntityHandle>("EntityHandle", "getGuid", &EntityHandle::GetID, "isValid", &EntityHandle::IsValid, "clear", [](EntityHandle& self) { self = EntityHandle(); });
		lua.set_function("ehandle", sol::overload([]() { return EntityHandle(); }, [](const std::string& guid) { return EntityHandle(guid); }));


		// Factory for entity handle vector
		// using EntityVector = ;

		using EntityVector = std::vector<EntityHandle>;

		lua.new_usertype<EntityVector>(
		    "EntityVector",
		    sol::constructors<EntityVector()>(),
		    "push_back",
		    static_cast<void (EntityVector::*)(const EntityHandle&)>(&EntityVector::push_back),
		    "size",
		    &EntityVector::size,
		    // indexing operator (Lua is 1-based, so shift indices)
		    sol::meta_function::index,
		    [](EntityVector& self, std::size_t i) -> EntityHandle& {
			    if (i == 0 || i > self.size()) throw std::out_of_range("Index out of range");
			    return self[i - 1];
		    },
		    sol::meta_function::new_index,
		    [](EntityVector& self, std::size_t i, const EntityHandle& value) {
			    if (i == 0 || i > self.size()) throw std::out_of_range("Index out of range");
			    self[i - 1] = value;
		    });

		using MaterialHandle = AssetHandle<Material>;
		using MaterialVector = std::vector<MaterialHandle>;


		//		lua.new_usertype<MaterialVector>(
		//		    "MaterialVector",
		//
		//		    sol::constructors<MaterialVector()>(),
		//
		//		    "push_back",
		//		    static_cast<void (MaterialVector::*)(const MaterialHandle&)>(&MaterialVector::push_back),
		//
		//		    "size",
		//		    &MaterialVector::size,
		//
		//		    // 1-based indexing for Lua
		//		    sol::meta_function::index,
		//		    [](MaterialVector& self, std::size_t i) -> MaterialHandle& {
		//			    if (i == 0 || i > self.size()) throw std::out_of_range("Index out of range");
		//			    return self[i - 1]; // return by REF, important
		//		    },
		//
		//		    sol::meta_function::new_index,
		//		    [](MaterialVector& self, std::size_t i, const MaterialHandle& value) {
		//			    if (i == 0 || i > self.size()) throw std::out_of_range("Index out of range");
		//			    self[i - 1] = value;
		//		    });
	}


} // namespace Engine::Components