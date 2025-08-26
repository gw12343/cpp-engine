//
// Created by gabe on 8/16/25.
//
#include "JSONSceneLoader.h"

#include <cereal/archives/json.hpp>
#include <cereal/types/optional.hpp>
#include <cereal/types/common.hpp>

#include <iostream>
#include "components/AllComponents.h"
#include "components/Components.h"
#include "core/SceneManager.h"

#include <fstream>
#include <optional>


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

	void JSONSceneLoader::SerializeScene(const AssetHandle<Scene>& sceneRef, const std::string& path)
	{
		std::ofstream             os(path);
		cereal::JSONOutputArchive archive(os);

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

	std::unique_ptr<Scene> JSONSceneLoader::LoadFromFile(const std::string& path)
	{
		std::unique_ptr<Scene> scene = GetSceneManager().CreateScene(path);

		std::ifstream            is(path);
		cereal::JSONInputArchive archive(is);

		std::vector<SerializedEntity> entities;
		archive(cereal::make_nvp("entities", entities));
		std::vector<Entity>            loaded_entities;
		std::map<EntityHandle, Entity> loaded_entities_map;

		for (auto& se : entities) {
			auto e = scene->GetRegistry()->create();
			scene->GetRegistry()->emplace<Engine::Components::EntityMetadata>(e, se.meta);
			Entity entity(e, scene.get());
			loaded_entities.push_back(entity);
			loaded_entities_map[EntityHandle(se.meta.guid)] = entity;

#define X(type, name, fancy)                                                                                                                                                                                                                   \
	if (se.name.has_value()) entity.AddComponent<type>(se.name.value());
			COMPONENT_LIST
#undef X
		}

		scene->m_entityList = loaded_entities;
		scene->m_entityMap  = loaded_entities_map;
		return scene;
	}


} // namespace Engine