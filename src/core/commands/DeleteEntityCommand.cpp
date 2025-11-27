#include "DeleteEntityCommand.h"
#include "core/Entity.h"
#include "core/EngineData.h"
#include "components/AllComponents.h"
#include "components/impl/EntityMetadataComponent.h"

namespace Engine {

	DeleteEntityCommand::DeleteEntityCommand(const EntityHandle& entityHandle) : m_entityHandle(entityHandle)
	{
		CreateBackup();
	}

	void DeleteEntityCommand::Execute()
	{
		Entity entity = GetCurrentScene()->Get(m_entityHandle);
		if (!entity.IsValid()) {
			GetDefaultLogger()->warn("DeleteEntityCommand::Execute: Entity not found");
			return;
		}

		// Destroy immediately (like inspector delete button)
		entity.Destroy();
		m_wasExecuted = true;
	}

	void DeleteEntityCommand::Undo()
	{
		if (!m_wasExecuted) {
			return;
		}

		RestoreFromBackup();
		m_wasExecuted = false;
	}

	std::string DeleteEntityCommand::GetDescription() const
	{
		return "Delete Entity";
	}

	void DeleteEntityCommand::CreateBackup()
	{
		Entity entity = GetCurrentScene()->Get(m_entityHandle);
		if (!entity.IsValid()) {
			GetDefaultLogger()->error("DeleteEntityCommand: Cannot create backup of invalid entity");
			return;
		}

		auto& meta = entity.GetComponent<Components::EntityMetadata>();
		m_entityName = meta.name;
		m_parentHandle = meta.parentEntity;

		// Create a backup entity by cloning (same approach as CloneEntityCommand)
		Entity backup = Entity::Create("__BACKUP__" + m_entityName, GetCurrentScene());
		
		// Copy all components to backup
#define X(type, name, fancy) \
		if (entity.HasComponent<type>()) { \
			backup.AddComponent<type>(entity.GetComponent<type>()); \
		}
		COMPONENT_LIST
#undef X

		m_backupEntityHandle = backup.GetEntityHandle();

		// Recursively backup children  
		for (const auto& childHandle : meta.children) {
			Entity child = GetCurrentScene()->Get(childHandle);
			if (child.IsValid()) {
				auto childCmd = std::make_unique<DeleteEntityCommand>(childHandle);
				m_childCommands.push_back(std::move(childCmd));
			}
		}

		GetDefaultLogger()->info("DeleteEntityCommand: Created backup for '{}' with {} children", 
		                         m_entityName, m_childCommands.size());
	}

	void DeleteEntityCommand::RestoreFromBackup()
	{
		Entity backup = GetCurrentScene()->Get(m_backupEntityHandle);
		if (!backup.IsValid()) {
			GetDefaultLogger()->error("DeleteEntityCommand::Undo: Backup entity no longer exists!");
			return;
		}

		// Create new entity with original name
		Entity restored = Entity::Create(m_entityName, GetCurrentScene());

		// Copy all components from backup
#define X(type, name, fancy) \
		if (backup.HasComponent<type>()) { \
			restored.AddComponent<type>(backup.GetComponent<type>()); \
		}
		COMPONENT_LIST
#undef X

		// Register in scene map with original handle
		GetCurrentScene()->m_entityMap[m_entityHandle] = restored;

		// Restore parent relationship
		if (m_parentHandle.IsValid()) {
			restored.SetParent(m_parentHandle);
		}

		// Restore children
		for (auto& childCmd : m_childCommands) {
			childCmd->Undo();
			// Set the child's parent to be this restored entity
			Entity child = GetCurrentScene()->Get(childCmd->m_entityHandle);
if (child.IsValid()) {
				child.SetParent(restored.GetEntityHandle());
			}
		}

		// Cleanup backup entity
		CleanupBackup();

		GetDefaultLogger()->info("DeleteEntityCommand::Undo: Fully restored entity '{}' with all components and {} children", 
		                         m_entityName, m_childCommands.size());
	}

	void DeleteEntityCommand::CleanupBackup()
	{
		Entity backup = GetCurrentScene()->Get(m_backupEntityHandle);
		if (backup.IsValid()) {
			backup.Destroy();
		}
	}

} // namespace Engine
