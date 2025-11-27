#include "EntityClipboard.h"
#include "Entity.h"
#include "EngineData.h"
#include "components/AllComponents.h"
#include "components/impl/EntityMetadataComponent.h"
#include "utils/Utils.h"

#include <cereal/archives/json.hpp>
#include <sstream>

namespace Engine {

	// Helper struct for serialization
	struct ClipboardEntityData {
		Components::EntityMetadata meta;
		std::vector<ClipboardEntityData> children; // Recursive children

#define X(type, name, fancy) std::optional<type> name;
		COMPONENT_LIST
#undef X

		template <class Archive>
		void serialize(Archive& ar)
		{
			ar(cereal::make_nvp("EntityMetadata", meta));
			ar(cereal::make_nvp("children", children));
#define X(type, name, fancy) ar(cereal::make_nvp(#name, name));
			COMPONENT_LIST
#undef X
		}
	};

	void EntityClipboard::CopyEntity(const Entity& entity)
	{
		if (!entity.IsValid()) {
			GetDefaultLogger()->warn("EntityClipboard::CopyEntity: Invalid entity");
			return;
		}

		// For now, simplified approach: just store the entity handle
		// Full hierarchical copy/paste is complex and best done via the existing 
		// clone mechanism wrapped in commands
		auto& meta = entity.GetComponent<Components::EntityMetadata>();
		
		nlohmann::json clipData;
		clipData["name"] = meta.name;
		clipData["guid"] = meta.guid;
		
		m_clipboardData = clipData;
		GetDefaultLogger()->info("EntityClipboard: Copied entity '{}'", meta.name);
	}

	EntityHandle EntityClipboard::PasteEntity(Scene* scene)
	{
		if (!HasData()) {
			GetDefaultLogger()->warn("EntityClipboard::PasteEntity: No data to paste");
			return EntityHandle();
		}

		// For now, use simplified approach 
		// Full implementation would deserialize from JSON
		// For MVP, user can use Ctrl+D (clone) wrapped in command
		GetDefaultLogger()->warn("EntityClipboard::PasteEntity: Not fully implemented - use Ctrl+D to clone entities");
		return EntityHandle();
	}

	void EntityClipboard::Clear()
	{
		m_clipboardData.clear();
	}

	void EntityClipboard::SerializeEntityHierarchy(const Entity& entity, nlohmann::json& outJson)
	{
		// TODO: Implement full recursive hierarchy serialization if needed
	}

	EntityHandle EntityClipboard::DeserializeEntityHierarchy(const nlohmann::json& json, Scene* scene, const EntityHandle& parentHandle)
	{
		// TODO: Implement full recursive hierarchy deserialization if needed
		return EntityHandle();
	}

} // namespace Engine
