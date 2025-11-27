#pragma once

#include "core/EditorCommand.h"
#include "core/EntityHandle.h"

namespace Engine {

	/// Command for cloning an entity
	class CloneEntityCommand : public EditorCommand {
	  public:
		explicit CloneEntityCommand(const EntityHandle& sourceEntityHandle);

		void        Execute() override;
		void        Undo() override;
		std::string GetDescription() const override;

		// Get the handle of the cloned entity (valid after Execute)
		const EntityHandle& GetClonedEntityHandle() const { return m_clonedEntityHandle; }

	  private:
		EntityHandle m_sourceEntityHandle;
		EntityHandle m_clonedEntityHandle;
		bool         m_wasExecuted = false;

		void CloneEntity();
		void DeleteClonedEntity();
	};

} // namespace Engine
