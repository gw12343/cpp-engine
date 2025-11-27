#pragma once

#include "core/EditorCommand.h"
#include "core/EntityHandle.h"
#include "nlohmann/json.hpp"
#include <cereal/types/optional.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>
#include <memory>
#include <vector>

namespace Engine {

	/// Command for deleting an entity (and restoring it on undo) using cereal JSON serialization
	class DeleteEntityCommand : public EditorCommand {
	  public:
		explicit DeleteEntityCommand(const EntityHandle& entityHandle);

		void        Execute() override;
		void        Undo() override;
		std::string GetDescription() const override;

	  private:
		EntityHandle    m_entityHandle;
		std::string     m_entityName;
		EntityHandle    m_parentHandle;
		bool            m_wasExecuted = false;

		// Serialized JSON representation of the entity and its hierarchy
		nlohmann::json  m_serializedEntity;

		void SerializeEntityRecursive(const Entity& entity, nlohmann::json& out);
		Entity RestoreEntityRecursive(const nlohmann::json& data, const EntityHandle& parentHandle = {});
	};

} // namespace Engine
	};

} // namespace Engine
