//
// Created by gabe on 8/16/25.
//
#include "JSONSceneLoader.h"

#include <cereal/archives/json.hpp>
#include <cereal/types/optional.hpp>
#include <cereal/types/common.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/memory.hpp>

#include "assets/SerializedEntity.h"
#include "components/AllComponents.h"
#include "components/Components.h"
#include "core/SceneManager.h"
#include "core/EngineData.h"

#include <fstream>
#include <optional>
#include <sstream>
#include <cstdio>
#include <nlohmann/json.hpp>


namespace Engine {

	void JSONSceneLoader::SerializeScene(const SceneHandle& sceneRef, const std::string& path)
	{
		std::ofstream             os(path);
		cereal::JSONOutputArchive archive(os);

		std::vector<SerializedEntity> entities;

		Scene* scene = GetAssetManager().Get(sceneRef);

		auto registry = scene->GetRegistry();

		// Collect all entities with their metadata
		registry->view<Components::EntityMetadata>().each([&](auto entity, auto meta) {
			SerializedEntity se;
			se.meta = meta;
			// Now loop over all the components in the list
#define X(type, name, fancy)                                                                                                                                                                                                                   \
	if (registry->all_of<type>(entity)) se.name = registry->get<type>(entity);
			COMPONENT_LIST
#undef X

			entities.push_back(std::move(se));
		});

		// Sort entities by GUID to ensure deterministic serialization for version control
		std::sort(entities.begin(), entities.end(), [](const SerializedEntity& a, const SerializedEntity& b) {
			return a.meta.guid < b.meta.guid;
		});

		archive(cereal::make_nvp("entities", entities));
	}

	namespace {
		// Always print cereal/rapidjson failures to the log *and* stderr so they
		// are visible even if spdlog sinks are not yet fully configured.
		void LogCerealError(const char* phase, const std::string& path, const char* what)
		{
			const std::string msg = std::string("[cereal] ") + phase + " '" + path + "': " + (what ? what : "(null)");
			std::fprintf(stderr, "%s\n", msg.c_str());
			std::fflush(stderr);
			GetDefaultLogger()->error("{}", msg);
		}
	} // namespace

	std::unique_ptr<Scene> JSONSceneLoader::LoadFromFile(const std::string& path)
	{
		GetDefaultLogger()->info("[cereal] Loading JSON scene: {}", path);

		std::unique_ptr<Scene> scene = GetSceneManager().CreateScene(path);

		std::ifstream is(path);
		if (!is) {
			LogCerealError("open failed", path, "could not open file for reading");
			return scene;
		}

		std::vector<SerializedEntity> entities;
		try {
			nlohmann::json j;
			is >> j;
			EnsureSerializedEntityKeys(j);
			std::stringstream ss;
			ss << j.dump();
			cereal::JSONInputArchive archive(ss);
			archive(cereal::make_nvp("entities", entities));
			GetDefaultLogger()->info("[cereal] Parsed {} entities from {}", entities.size(), path);
		}
		catch (const cereal::Exception& e) {
			// Includes cereal::RapidJSONException ("rapidjson internal assertion failure: ...")
			// and NVP-not-found / type errors.
			LogCerealError("JSONInputArchive exception", path, e.what());
			return scene;
		}
		catch (const std::exception& e) {
			LogCerealError("std::exception during deserialize", path, e.what());
			return scene;
		}
		catch (...) {
			LogCerealError("unknown exception during deserialize", path, "non-std exception");
			return scene;
		}

		std::vector<Entity>            loaded_entities;
		std::map<EntityHandle, Entity> loaded_entities_map;

		for (size_t i = 0; i < entities.size(); ++i) {
			auto& se = entities[i];
			// Copy before COMPONENT_LIST macros: parameter `name` would rewrite se.meta.name
			// into se.meta.LuaScript / se.meta.Transform / etc.
			const std::string entityName = se.meta.name;
			const std::string entityGuid = se.meta.guid;
			try {
				auto e = scene->GetRegistry()->create();
				scene->GetRegistry()->emplace<Engine::Components::EntityMetadata>(e, se.meta);
				Entity entity(e, scene.get());
				loaded_entities.push_back(entity);
				loaded_entities_map[EntityHandle(entityGuid)] = entity;

#define X(type, name, fancy)                                                                                                                                                                                                                   \
	if (se.name.has_value()) {                                                                                                                                                                                                                 \
		try {                                                                                                                                                                                                                                  \
			entity.AddComponent<type>(se.name.value());                                                                                                                                                                                        \
		}                                                                                                                                                                                                                                      \
		catch (const std::exception& ex) {                                                                                                                                                                                                     \
			std::string ctx = std::string("AddComponent ") + #name + " entity=" + entityName + " [" + entityGuid + "]";                                                                                                                        \
			LogCerealError(ctx.c_str(), path, ex.what());                                                                                                                                                                                      \
		}                                                                                                                                                                                                                                      \
	}
				COMPONENT_LIST
#undef X
			}
			catch (const cereal::Exception& e) {
				LogCerealError(
				    ("entity index " + std::to_string(i) + " name=" + entityName + " guid=" + entityGuid).c_str(),
				    path,
				    e.what());
			}
			catch (const std::exception& e) {
				LogCerealError(
				    ("entity index " + std::to_string(i) + " name=" + entityName + " guid=" + entityGuid).c_str(),
				    path,
				    e.what());
			}
		}

		scene->m_entityList = loaded_entities;
		scene->m_entityMap  = loaded_entities_map;
		GetDefaultLogger()->info("[cereal] Scene load finished: {} entities in registry", loaded_entities.size());
		return scene;
	}


} // namespace Engine