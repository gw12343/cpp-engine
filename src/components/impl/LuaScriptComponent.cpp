//
// Created by gabe on 6/24/25.
//
#include "components/Components.h"
#include "components/impl/LuaScriptComponent.h"

#include "core/Entity.h"

#include <utility>
#include "rendering/particles/ParticleManager.h"
#include "animation/AnimationManager.h"
#include "scripting/ScriptManager.h"



namespace Engine::Components {
	void LuaScript::UnsubscribeAll()
	{
		for (uint32_t id : subscriptionIDs) {
			GetScriptManager().eventBus.Unsubscribe(id);
		}
		subscriptionIDs.clear();
	}

	void LuaScript::OnRemoved(Entity& entity)
	{
		UnsubscribeAll();

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
#define ASSET_VEC_CHK(nme, typ)                                                                                                                                                                                                                \
	else if (obj.is<std::vector<AssetHandle<typ>>>())                                                                                                                                                                                          \
	{                                                                                                                                                                                                                                          \
		auto handle = obj.as<std::vector<AssetHandle<typ>>>();                                                                                                                                                                                 \
		if (LeftLabelAssetVector##nme(key.c_str(), handle)) {                                                                                                                                                                                  \
			variables[key]    = handle;                                                                                                                                                                                                        \
			cppVariables[key] = handle;                                                                                                                                                                                                        \
			SyncFromLua();                                                                                                                                                                                                                     \
		}                                                                                                                                                                                                                                      \
	}

							ASSET_VEC_CHK(Texture, Texture)
							ASSET_VEC_CHK(Model, Rendering::Model)
							ASSET_VEC_CHK(Material, Material)
							ASSET_VEC_CHK(Scene, Scene)
							ASSET_VEC_CHK(Terrain, Terrain::TerrainTile)
							ASSET_VEC_CHK(Particle, Particle)
							ASSET_VEC_CHK(Sound, Audio::SoundBuffer)

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
			else if (kv.second.is<TextureHandle>()) {
				cppVariables[key] = kv.second.as<TextureHandle>();
			}
			else if (kv.second.is<ModelHandle>()) {
				cppVariables[key] = kv.second.as<ModelHandle>();
			}
			else if (kv.second.is<MaterialHandle>()) {
				cppVariables[key] = kv.second.as<MaterialHandle>();
			}
			else if (kv.second.is<SceneHandle>()) {
				cppVariables[key] = kv.second.as<SceneHandle>();
			}
			else if (kv.second.is<TerrainHandle>()) {
				cppVariables[key] = kv.second.as<TerrainHandle>();
			}
			else if (kv.second.is<ParticleHandle>()) {
				cppVariables[key] = kv.second.as<ParticleHandle>();
			}
			else if (kv.second.is<SoundHandle>()) {
				cppVariables[key] = kv.second.as<SoundHandle>();
			}
			else if (kv.second.is<EntityHandle>()) {
				cppVariables[key] = kv.second.as<EntityHandle>();
			}
			else if (kv.second.is<EntityHandleList>()) {
				cppVariables[key] = kv.second.as<EntityHandleList>();
			}
			else if (kv.second.is<TextureHandleList>()) {
				cppVariables[key] = kv.second.as<TextureHandleList>();
			}
			else if (kv.second.is<ModelHandleList>()) {
				cppVariables[key] = kv.second.as<ModelHandleList>();
			}
			else if (kv.second.is<MaterialHandleList>()) {
				cppVariables[key] = kv.second.as<MaterialHandleList>();
			}
			else if (kv.second.is<SceneHandleList>()) {
				cppVariables[key] = kv.second.as<SceneHandleList>();
			}
			else if (kv.second.is<TerrainHandleList>()) {
				cppVariables[key] = kv.second.as<TerrainHandleList>();
			}
			else if (kv.second.is<ParticleHandleList>()) {
				cppVariables[key] = kv.second.as<ParticleHandleList>();
			}
			else if (kv.second.is<SoundHandleList>()) {
				cppVariables[key] = kv.second.as<SoundHandleList>();
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
						[&, key = key](auto&& arg) {
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

		GetScriptManager().log->warn("Script requested an invalid entity: {}", handle.GetID());
		return {};
	}

	void LuaScript::LoadScript(Entity& entity, std::string path)
	{
		this->scriptPath = std::move(path);

		// Clear old Lua environment and bound functions in case we're reloading
		env                  = sol::environment();
		start                = sol::function();
		update               = sol::function();
		lateUpdate           = sol::function();
		collisionEnter       = sol::function();
		playerCollisionEnter = sol::function();
		variables            = sol::table();
		
		// Clear any existing subscriptions from the previous script instance
		UnsubscribeAll();

		if (scriptPath.empty()) return;

		// Create a fresh Lua environment
		env = sol::environment(GetScriptManager().lua, sol::create, GetScriptManager().lua.globals());

		// Inject gameObject
		env["gameObject"] = entity;

		// Inject custom subscribe function to track subscriptions
		env["subscribe"] = [this](const std::string& eventName, sol::function callback) {
			uint32_t id = GetScriptManager().eventBus.Subscribe(eventName, callback);
			this->subscriptionIDs.push_back(id);
			return id;
		};


		try {
			// Load the script into the environment
#ifdef GAME_BUILD
			// In game builds, load compiled .luac bytecode instead of source
			std::string loadPath = scriptPath;
			if (loadPath.size() >= 4 && loadPath.substr(loadPath.size() - 4) == ".lua") {
				loadPath = loadPath.substr(0, loadPath.size() - 4) + ".luac";
			}
			GetScriptManager().log->info("Loading compiled script: {}", loadPath);
			GetScriptManager().lua.script_file(loadPath, env);
#else
			// In editor mode, load source .lua files
			GetScriptManager().lua.script_file(scriptPath, env);
#endif

			// Bind lifecycle functions
			start                = env["Start"];
			update               = env["Update"];
			lateUpdate           = env["LateUpdate"];
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
			GetScriptManager().log->error("[LuaScript] Error in  (original path){}: {}", scriptPath, err.what());
		}
	}




} // namespace Engine::Components