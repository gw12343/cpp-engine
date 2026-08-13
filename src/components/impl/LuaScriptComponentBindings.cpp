//
// Created by Gabe on 6/4/2026.
//

#include "components/Components.h"
#include "components/impl/LuaScriptComponent.h"

#include "core/Entity.h"

#include "animation/AnimationManager.h"
#include "scripting/ScriptManager.h"


namespace Engine
{
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
		lua.new_usertype<TextureHandle>("TextureHandle", "getGuid", &TextureHandle::GetID, "isValid", &TextureHandle::IsValid, "clear", [](TextureHandle& self) { self = TextureHandle(); });

		lua.new_usertype<ModelHandle>(
			"ModelHandle", "getGuid", &ModelHandle::GetID, "isValid", &ModelHandle::IsValid, "clear", [](ModelHandle& self) { self = ModelHandle(); });

		lua.new_usertype<MaterialHandle>("MaterialHandle", "getGuid", &MaterialHandle::GetID, "isValid", &MaterialHandle::IsValid, "clear", [](MaterialHandle& self) { self = MaterialHandle(); });

		lua.new_usertype<SceneHandle>("SceneHandle", "getGuid", &SceneHandle::GetID, "isValid", &SceneHandle::IsValid, "clear", [](SceneHandle& self) { self = SceneHandle(); });

		lua.new_usertype<TerrainHandle>(
			"TerrainTileHandle", "getGuid", &TerrainHandle::GetID, "isValid", &TerrainHandle::IsValid, "clear", [](TerrainHandle& self) {
				self = TerrainHandle();
			});

		lua.new_usertype<ParticleHandle>("ParticleHandle", "getGuid", &ParticleHandle::GetID, "isValid", &ParticleHandle::IsValid, "clear", [](ParticleHandle& self) { self = ParticleHandle(); });

		lua.new_usertype<SoundHandle>(
			"SoundHandle", "getGuid", &SoundHandle::GetID, "isValid", &SoundHandle::IsValid, "clear", [](SoundHandle& self) { self = SoundHandle(); });

		lua.new_usertype<AnimationHandle>(
			"AnimationHandle",
			sol::constructors<AnimationHandle(), AnimationHandle(const std::string&)>(),
			"getGuid", &AnimationHandle::GetID,
			"isValid", &AnimationHandle::IsValid,
			"clear", [](AnimationHandle& self) { self = AnimationHandle(); },
			sol::meta_function::equal_to,
			[](const AnimationHandle& a, const AnimationHandle& b) { return a == b; });

		// SkeletonReference is the preferred name; SkeletonHandle is an alias.
		lua.new_usertype<SkeletonReference>(
			"SkeletonReference",
			sol::constructors<SkeletonReference(), SkeletonReference(const std::string&)>(),
			"getGuid", &SkeletonReference::GetID,
			"isValid", &SkeletonReference::IsValid,
			"clear", [](SkeletonReference& self) { self = SkeletonReference(); },
			sol::meta_function::equal_to,
			[](const SkeletonReference& a, const SkeletonReference& b) { return a == b; });
		// Alias type name for scripts that prefer "SkeletonHandle"
		lua["SkeletonHandle"] = lua["SkeletonReference"];

		// Factory functions for creating asset handles (empty, or from GUID string)
		lua.set_function("tex", sol::overload([]() { return TextureHandle(); }, [](const std::string& g) { return TextureHandle(g); }));
		lua.set_function("model", sol::overload([]() { return ModelHandle(); }, [](const std::string& g) { return ModelHandle(g); }));
		lua.set_function("material", sol::overload([]() { return MaterialHandle(); }, [](const std::string& g) { return MaterialHandle(g); }));
		lua.set_function("scene", sol::overload([]() { return SceneHandle(); }, [](const std::string& g) { return SceneHandle(g); }));
		lua.set_function("terrainTile", sol::overload([]() { return TerrainHandle(); }, [](const std::string& g) { return TerrainHandle(g); }));
		lua.set_function("particle", sol::overload([]() { return ParticleHandle(); }, [](const std::string& g) { return ParticleHandle(g); }));
		lua.set_function("sound", sol::overload([]() { return SoundHandle(); }, [](const std::string& g) { return SoundHandle(g); }));
		lua.set_function("animation", sol::overload([]() { return AnimationHandle(); }, [](const std::string& g) { return AnimationHandle(g); }));
		lua.set_function("skeleton", sol::overload([]() { return SkeletonReference(); }, [](const std::string& g) { return SkeletonReference(g); }));

		// Factory for entity handle
		lua.new_usertype<EntityHandle>("EntityHandle", "getGuid", &EntityHandle::GetID, "isValid", &EntityHandle::IsValid, "clear", [](EntityHandle& self) { self = EntityHandle(); });
		lua.set_function("ehandle", sol::overload([]() { return EntityHandle(); }, [](const std::string& guid) { return EntityHandle(guid); }));



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
		BIND_ASSET_VECTOR("AnimationVector", Animation)
		BIND_ASSET_VECTOR("SkeletonVector", Skeleton)
	}
}