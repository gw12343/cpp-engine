#include "CloneEntityCommand.h"
#include "core/Entity.h"
#include "core/EngineData.h"
#include "components/AllComponents.h"
#include "components/impl/EntityMetadataComponent.h"

namespace Engine {

	CloneEntityCommand::CloneEntityCommand(const EntityHandle& sourceEntityHandle) : m_sourceEntityHandle(sourceEntityHandle)
	{
	}

	void CloneEntityCommand::Execute()
	{
		CloneEntity();
		m_wasExecuted = true;
	}

	void CloneEntityCommand::Undo()
	{
		if (!m_wasExecuted) {
			return;
		}

		DeleteClonedEntity();
		m_wasExecuted = false;
	}

	std::string CloneEntityCommand::GetDescription() const
	{
		return "Clone Entity";
	}

	void CloneEntityCommand::CloneEntity()
	{
		Entity sourceEntity = GetCurrentScene()->Get(m_sourceEntityHandle);
		if (!sourceEntity.IsValid()) {
			GetDefaultLogger()->error("CloneEntityCommand: Source entity not found");
			return;
		}

		// Get entity name and create clone name
		std::string newName = sourceEntity.GetComponent<Components::EntityMetadata>().name;
		if (newName.rfind("Copy of ", 0) != 0) {
			newName = "Copy of " + newName;
		}

		// Create new entity
		Entity copy = Entity::Create(newName, sourceEntity.m_scene);

		// Copy all components (reuse existing clone logic from SceneViewWindow.cpp)
#define X(type, name, fancy)                                                                                                                                                                                                                   \
	if (sourceEntity.HasComponent<type>()) {                                                                                                                                                                                                \
		copy.AddComponent<type>(sourceEntity.GetComponent<type>());                                                                                                                                                                         \
	}
		COMPONENT_LIST
#undef X

		// Store the cloned entity handle
		m_clonedEntityHandle = copy.GetEntityHandle();
	}

	void CloneEntityCommand::DeleteClonedEntity()
	{
		Entity clonedEntity = GetCurrentScene()->Get(m_clonedEntityHandle);
		if (clonedEntity.IsValid()) {
			clonedEntity.Destroy();
		}
	}

} // namespace Engine
