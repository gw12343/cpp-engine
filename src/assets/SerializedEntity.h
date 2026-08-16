#pragma once

#include <cereal/cereal.hpp>
#include <cereal/types/common.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/optional.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/unordered_map.hpp>
#include <cereal/types/variant.hpp>
#include <cereal/types/vector.hpp>

#include <nlohmann/json.hpp>
#include <optional>
#include <string>

#include "assets/AssetHandle.h"
#include "components/AllComponents.h"
#include "core/EntityHandle.h"

namespace glm {
	template <class Archive>
	void serialize(Archive& ar, vec3& v)
	{
		ar(cereal::make_nvp("x", v.x), cereal::make_nvp("y", v.y), cereal::make_nvp("z", v.z));
	}

	template <class Archive>
	void serialize(Archive& ar, vec4& v)
	{
		ar(cereal::make_nvp("x", v.x), cereal::make_nvp("y", v.y), cereal::make_nvp("z", v.z), cereal::make_nvp("w", v.w));
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
		std::string guid = handle.GetID();
		ar(cereal::make_nvp("guid", guid));
		if constexpr (Archive::is_loading::value) {
			handle = AssetHandle<T>(guid);
		}
	}

	template <class Archive>
	void serialize(Archive& ar, EntityHandle& handle)
	{
		std::string guid = handle.GetID();
		ar(cereal::make_nvp("guid", guid));
		if constexpr (Archive::is_loading::value) {
			handle = EntityHandle(guid);
		}
	}

	struct SerializedEntity {
		Engine::Components::EntityMetadata meta;

#define X(type, name, fancy) std::optional<type> name;
		COMPONENT_LIST
#undef X

		template <class Archive>
		void serialize(Archive& ar)
		{
			ar(cereal::make_nvp("EntityMetadata", meta));
#define X(type, name, fancy) ar(cereal::make_nvp(#name, name));
			COMPONENT_LIST
#undef X
		}
	};

	// Old scene / prefab JSON is missing keys for components added later.
	// Inject cereal's optional-empty shape so JSONInputArchive can read them.
	inline void EnsureSerializedEntityKeys(nlohmann::json& root)
	{
		if (!root.contains("entities") || !root["entities"].is_array()) {
			return;
		}
		for (auto& entity : root["entities"]) {
			if (!entity.is_object()) continue;
#define X(type, name, fancy)                                                                                                                                                                                                                   \
			if (!entity.contains(#name)) {                                                                                                                                                                                                     \
				entity[#name] = nlohmann::json{{"nullopt", true}};                                                                                                                                                                              \
			}
			COMPONENT_LIST
#undef X
		}
	}
} // namespace Engine
