//
// Created by gabe on 8/16/25.
//
#include "BinarySceneLoader.h"

#include <cereal/cereal.hpp>
#include <cereal/archives/binary.hpp>


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

namespace glm {
	template <class Archive>
	void serialize(Archive& ar, vec3& v)
	{
		ar(cereal::make_nvp("x", v.x), cereal::make_nvp("y", v.y), cereal::make_nvp("z", v.z));
	}

	template <class Archive>
	void serialize(Archive& ar, quat& q)
	{
		ar(cereal::make_nvp("x", q.x), cereal::make_nvp("y", q.y), cereal::make_nvp("z", q.z), cereal::make_nvp("w", q.w));
	}
} // namespace glm

namespace JPH {

	template <class Archive>
	void serialize(Archive& ar, Vec3& v)
	{
		float x = v.GetX();
		float y = v.GetY();
		float z = v.GetZ();
		ar(cereal::make_nvp("x", x), cereal::make_nvp("y", y), cereal::make_nvp("z", z));
		if constexpr (Archive::is_loading::value) {
			v.Set(x, y, z);
		}
	}

	template <class Archive>
	void serialize(Archive& ar, Quat& q)
	{
		float x = q.GetX();
		float y = q.GetY();
		float z = q.GetZ();
		float w = q.GetW();
		ar(cereal::make_nvp("x", x), cereal::make_nvp("y", y), cereal::make_nvp("z", z), cereal::make_nvp("w", w));
		if constexpr (Archive::is_loading::value) {
			q.Set(x, y, z, w);
		}
	}
} // namespace JPH


namespace Engine {

	template <class Archive, typename T>
	void serialize(Archive& ar, AssetHandle<T>& handle)
	{
		// Serialize the internal GUID string
		std::string guid = handle.GetID();

		ar(cereal::make_nvp("guid", guid));

		if constexpr (Archive::is_loading::value) {
			handle = AssetHandle<T>(guid);
		}
	}

	template <class Archive>
	void serialize(Archive& ar, EntityHandle& handle)
	{
		// Serialize the internal GUID string
		std::string guid = handle.GetID();

		ar(cereal::make_nvp("guid", guid));

		if constexpr (Archive::is_loading::value) {
			handle = EntityHandle(guid);
		}
	}


	struct SerializedEntity {
		Engine::Components::EntityMetadata meta;

		// add all components as std::optional<T>
#define X(type, name, fancy) std::optional<type> name;
		COMPONENT_LIST
#undef X

		template <class Archive>
		void serialize(Archive& ar)
		{
			// First serialize EntityMetadata separately
			ar(cereal::make_nvp("EntityMetadata", meta));

// Now loop over all the components in the list
#define X(type, name, fancy) ar(cereal::make_nvp(#name, name));
			COMPONENT_LIST
#undef X

			//	);
		}
	};

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