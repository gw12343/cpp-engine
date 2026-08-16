//
// Created by gabe on 8/16/25.
//
#include "BinarySceneLoader.h"

#include <cereal/cereal.hpp>
#include <cereal/archives/binary.hpp>


#include "assets/SerializedEntity.h"
#include "components/AllComponents.h"
#include "core/SceneManager.h"
#include "core/EngineData.h"

#include <fstream>
#include <optional>
#include <cstdio>

#include <cereal/types/vector.hpp>
#include <cereal/types/optional.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/map.hpp>

#include "core/Scene.h"

namespace Engine {

	[[maybe_unused]] void BinarySceneLoader::SerializeScene(const SceneHandle& sceneRef, const std::string& path)
	{
		GetDefaultLogger()->info("Saving binary scene: {}", path);
		std::ofstream               os(path, std::ios::binary);
		cereal::BinaryOutputArchive archive(os);

		std::vector<SerializedEntity> entities;

		Scene* scene = GetAssetManager().Get(sceneRef);

		auto registry = scene->GetRegistry();

		registry->view<Components::EntityMetadata>().each([&](auto entity, auto meta) {
			SerializedEntity se;
			se.meta = meta;

			// Now loop over all the components in the list
#define X(type, name, fancy)                                                                                                                                                                                                                   \
	if (registry->all_of<type>(entity)) se.name = registry->get<type>(entity);
			COMPONENT_LIST
#undef X

			// if (registry.all_of<Components::Transform>(entity)) se.Transform = registry.get<Components::Transform>(entity);

			entities.push_back(std::move(se));
			// Repeat for other components...
		});
		archive(cereal::make_nvp("entities", entities));
	}


	std::unique_ptr<Scene> BinarySceneLoader::LoadFromFile(const std::string& path)
	{
		std::unique_ptr<Scene>         scene = GetSceneManager().CreateScene(path);
		std::vector<Entity>            loaded_entities;
		std::map<EntityHandle, Entity> loaded_entities_map;

		if (std::filesystem::exists(path)) {
			try {
				std::ifstream              is(path, std::ios::binary);
				cereal::BinaryInputArchive archive(is);

				std::vector<SerializedEntity> entities;
				archive(cereal::make_nvp("entities", entities));
				GetDefaultLogger()->info("[cereal] Binary scene parsed {} entities from {}", entities.size(), path);

				for (auto& se : entities) {
					// Copy before COMPONENT_LIST macros: parameter `name` would rewrite se.meta.name
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
			GetDefaultLogger()->error("[cereal] AddComponent {} entity={} [{}]: {}", #name, entityName, entityGuid, ex.what());                                                                                                                \
			std::fprintf(stderr, "[cereal] AddComponent %s entity=%s: %s\n", #name, entityName.c_str(), ex.what());                                                                                                                            \
		}                                                                                                                                                                                                                                      \
	}
						COMPONENT_LIST
#undef X
					}
					catch (const std::exception& ex) {
						GetDefaultLogger()->error("[cereal] entity {} [{}]: {}", entityName, entityGuid, ex.what());
						std::fprintf(stderr, "[cereal] entity %s: %s\n", entityName.c_str(), ex.what());
					}
				}
			}
			catch (const cereal::Exception& e) {
				GetDefaultLogger()->error("[cereal] BinaryInputArchive '{}': {}", path, e.what());
				std::fprintf(stderr, "[cereal] BinaryInputArchive '%s': %s\n", path.c_str(), e.what());
				std::fflush(stderr);
			}
			catch (const std::exception& e) {
				GetDefaultLogger()->error("[cereal] exception loading binary scene '{}': {}", path, e.what());
				std::fprintf(stderr, "[cereal] binary scene '%s': %s\n", path.c_str(), e.what());
				std::fflush(stderr);
			}
		}

		scene->m_entityList = loaded_entities;
		scene->m_entityMap  = loaded_entities_map;

		return scene;
	}


} // namespace Engine