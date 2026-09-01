#pragma once

#include "assets/SerializedEntity.h"
#include "core/Entity.h"

#include <memory>
#include <string>
#include <vector>

namespace Engine::UI {

	// Snapshot-based undo for editor entity edits.
	//
	// Continuous inspector / gizmo edits are coalesced via NotifyInteracting + Flush:
	// a drag or text field is one command. Discrete ops (spawn, delete, reparent,
	// rename, visibility) record immediately. New component fields are included
	// automatically because snapshots use each component's cereal serialize().

	class IUndoCommand {
	  public:
		virtual ~IUndoCommand() = default;
		virtual void        Undo()       = 0;
		virtual void        Redo()       = 0;
		virtual const char* Name() const = 0;
	};

	class UndoHistory {
	  public:
		static constexpr int kMaxCommands = 128;

		void        Undo();
		void        Redo();
		bool        CanUndo() const;
		bool        CanRedo() const;
		const char* UndoLabel() const;
		const char* RedoLabel() const;
		void        Clear();
		bool        IsApplying() const { return m_applying; }

		// Commit in-progress edits and freeze tracking while play-mode mutates the scene.
		void PrepareForPlay();
		// Scene has been restored from the play snapshot; keep the stack, recapture live state.
		void RestoreAfterStop();
		// Current stack index matches disk (Save). Undo/redo back here clears dirty.
		void MarkSaved();
		// Untitled / never-saved: always dirty until the next MarkSaved.
		void InvalidateSavePoint();

		// OR-accumulate during the frame (gizmo using, inspector widget, rename).
		void NotifyInteracting(bool interacting, const char* actionName = nullptr);
		// Call once after all editor UI. Coalesces property edits on `selected`.
		void Flush(Entity selected);

		// `root` already exists in the scene; undo destroys the captured subtree.
		void RecordSpawned(Entity root, const char* name);
		// Capture subtree, destroy it, record so undo restores original GUIDs.
		void DestroyAndRecord(Entity root, const char* name);

		// `entity` is already mutated; `before` is CaptureEntity() from before the change.
		void RecordModify(Entity entity, const SerializedEntity& before, const char* name);

		template <typename Fn>
		void Modify(Entity entity, const char* name, Fn&& fn)
		{
			if (!entity || !entity.IsValid() || m_applying) {
				fn();
				return;
			}
			SerializedEntity before = CaptureEntity(entity);
			fn();
			RecordModify(entity, before, name);
		}

		static SerializedEntity              CaptureEntity(Entity entity);
		static std::vector<SerializedEntity> CaptureSubtree(Entity root);

	  private:
		void Push(std::unique_ptr<IUndoCommand> cmd);
		void DiscardRedo();
		void CommitOpenSession();
		void CancelOpenSession();
		void SyncTracker(const SerializedEntity& state);
		void InvalidateTracker();
		void RefreshDirtyFlag();

		std::vector<std::unique_ptr<IUndoCommand>> m_stack;
		int                                        m_index       = -1;
		int                                        m_savedIndex  = -1;
		bool                                       m_saveValid   = true;
		bool                                       m_applying    = false;
		bool                                       m_interacting = false;
		std::string                                m_pendingName;

		std::string       m_trackGuid;
		SerializedEntity  m_lastClean;
		SerializedEntity  m_before;
		SerializedEntity  m_latest;
		std::string       m_openName = "Edit Entity";
		bool              m_open           = false;
		bool              m_lastCleanValid = false;
	};

	UndoHistory& GetUndo();

	// Current ImGui window (call before End) or a popup parented to it owns the active widget.
	bool CurrentWindowOwnsInteraction();

} // namespace Engine::UI
