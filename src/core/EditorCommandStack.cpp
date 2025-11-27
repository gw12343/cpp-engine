#include "EditorCommandStack.h"
#include "EngineData.h"

namespace Engine {

	EditorCommandStack::EditorCommandStack(size_t maxStackSize) : m_maxStackSize(maxStackSize)
	{
	}

	void EditorCommandStack::ExecuteCommand(std::unique_ptr<EditorCommand> command)
	{
		if (!command) {
			return;
		}

		GetDefaultLogger()->info("[EditorCommand] Executing: {}", command->GetDescription());

		// Execute the command
		command->Execute();

		// Add to undo stack
		m_undoStack.push_back(std::move(command));

		// Clear redo stack (new action invalidates redo history)
		m_redoStack.clear();

		// Enforce max stack size
		if (m_undoStack.size() > m_maxStackSize) {
			m_undoStack.pop_front();
		}
	}

	void EditorCommandStack::AddCommandWithoutExecute(std::unique_ptr<EditorCommand> command)
	{
		if (!command) {
			return;
		}

		GetDefaultLogger()->info("[EditorCommand] Adding (already executed): {}", command->GetDescription());

		// Don't execute - command was already executed
		// Just add to undo stack
		m_undoStack.push_back(std::move(command));

		// Clear redo stack (new action invalidates redo history)
		m_redoStack.clear();

		// Enforce max stack size
		if (m_undoStack.size() > m_maxStackSize) {
			m_undoStack.pop_front();
		}
	}

	void EditorCommandStack::Undo()
	{
		if (m_undoStack.empty()) {
			GetDefaultLogger()->warn("[EditorCommand] Cannot undo: stack is empty");
			return;
		}

		// Pop from undo stack
		auto command = std::move(m_undoStack.back());
		m_undoStack.pop_back();

		GetDefaultLogger()->info("[EditorCommand] Undoing: {}", command->GetDescription());

		// Undo the command
		command->Undo();

		// Move to redo stack
		m_redoStack.push_back(std::move(command));

		// Enforce max stack size on redo
		if (m_redoStack.size() > m_maxStackSize) {
			m_redoStack.pop_front();
		}
	}

	void EditorCommandStack::Redo()
	{
		if (m_redoStack.empty()) {
			GetDefaultLogger()->warn("[EditorCommand] Cannot redo: stack is empty");
			return;
		}

		// Pop from redo stack
		auto command = std::move(m_redoStack.back());
		m_redoStack.pop_back();

		GetDefaultLogger()->info("[EditorCommand] Redoing: {}", command->GetDescription());

		// Re-execute the command
		command->Execute();

		// Move back to undo stack
		m_undoStack.push_back(std::move(command));

		// Enforce max stack size
		if (m_undoStack.size() > m_maxStackSize) {
			m_undoStack.pop_front();
		}
	}

	void EditorCommandStack::Clear()
	{
		m_undoStack.clear();
		m_redoStack.clear();
		GetDefaultLogger()->info("EditorCommandStack: Cleared all undo/redo history");
	}

	std::string EditorCommandStack::GetUndoDescription() const
	{
		if (m_undoStack.empty()) {
			return "";
		}
		return m_undoStack.back()->GetDescription();
	}

	std::string EditorCommandStack::GetRedoDescription() const
	{
		if (m_redoStack.empty()) {
			return "";
		}
		return m_redoStack.back()->GetDescription();
	}

} // namespace Engine
