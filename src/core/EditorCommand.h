#pragma once

#include <string>

namespace Engine {

	/// Base interface for all undoable editor commands using the Command pattern
	class EditorCommand {
	  public:
		virtual ~EditorCommand() = default;

		/// Execute the command (perform the action)
		virtual void Execute() = 0;

		/// Undo the command (reverse the action)
		virtual void Undo() = 0;

		/// Get a description of this command for debugging/UI
		virtual std::string GetDescription() const = 0;
	};

} // namespace Engine
