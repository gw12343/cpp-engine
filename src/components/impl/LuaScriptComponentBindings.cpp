//
// Created by Gabe on 6/4/2026.
//
#include "components/Components.h"
#include "components/impl/LuaScriptComponent.h"

#include "core/Entity.h"

#include "animation/AnimationManager.h"
#include "scripting/ScriptManager.h"



void Components::LuaScript::AddBindings()
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

#define BIND_ASSET_VECTOR(Name, Type)                                                                                                                                                                                                          \
	{                                                                                                                                                                                                                                          \
		using VectorType = std::vector<AssetHandle<Type>>;                                                                                                                                                                                     \
		lua.new_usertype<VectorType>(Name, sol::constructors<VectorType()>(), "push_back", static_cast<void (VectorType::*)(const AssetHandle<Type>&)>(&VectorType::push_back), "size", &VectorType::size, sol::meta_function::index,          \
		    [](VectorType& self, std::size_t i) -> AssetHandle<Type>& {                                                                                                                                                                        \
			    if (i == 0 || i > self.size()) throw std::out_of_range("Index out of range");                                                                                                                                                  \
			    return self[i - 1];                                                                                                                                                                                                            \
		    },                                                                                                                                                                                                                                 \
		    sol::meta_function::new_index, [](VectorType& self, std::size_t i, const AssetHandle<Type>& value) {                                                                                                                               \
			    if (i == 0 || i > self.size()) throw std::out_of_range("Index out of range");                                                                                                                                                  \
			    self[i - 1] = value;                                                                                                                                                                                                           \
		    });                                                                                                                                                                                                                                \
	}

		BIND_ASSET_VECTOR("TextureVector", Texture)
		BIND_ASSET_VECTOR("ModelVector", Rendering::Model)
		BIND_ASSET_VECTOR("MaterialVector", Material)
		BIND_ASSET_VECTOR("SceneVector", Scene)
		BIND_ASSET_VECTOR("TerrainTileVector", Terrain::TerrainTile)
		BIND_ASSET_VECTOR("ParticleVector", Particle)
		BIND_ASSET_VECTOR("SoundVector", Audio::SoundBuffer)
	}