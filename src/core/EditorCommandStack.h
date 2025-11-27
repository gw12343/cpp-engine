#pragma once

#include "EditorCommand.h"
#include <deque>
#include <memory>

namespace Engine {

	/// Manages the undo/redo command stacks for the editor
	class EditorCommandStack {
	  public:
		EditorCommandStack(size_t maxStackSize = 50);

		/// Execute a command and add it to the undo stack
		void ExecuteCommand(std::unique_ptr<EditorCommand> command);

		/// Add an already-executed command to the undo stack (for operations like ImGuizmo that execute inline)
		void AddCommandWithoutExecute(std::unique_ptr<EditorCommand> command);

		/// Undo the last command
		void Undo();

		/// Redo the last undone command
		void Redo();

		/// Clear all undo/redo history
		void Clear();

		/// Check if undo is available
		bool CanUndo() const { return !m_undoStack.empty(); }

		/// Check if redo is available
		bool CanRedo() const { return !m_redoStack.empty(); }

		/// Get description of next undo operation
		std::string GetUndoDescription() const;

		/// Get description of next redo operation
		std::string GetRedoDescription() const;

	  private:
		std::deque<std::unique_ptr<EditorCommand>> m_undoStack;
		std::deque<std::unique_ptr<EditorCommand>> m_redoStack;
		size_t                                     m_maxStackSize;
	};

} // namespace Engine
