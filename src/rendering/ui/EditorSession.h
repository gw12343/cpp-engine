#pragma once

#include "core/Entity.h"

#include <string>
#include <unordered_set>

namespace Engine::UI {

	struct EditorSession {
		std::string scenePath          = "scenes/scene1.json";
		std::string playSnapshotPath   = "scenes/.play_snapshot.json";
		bool        dirty              = false;
		bool        playSnapshotValid  = false;

		bool showConsole        = true;
		bool showHierarchy      = true;
		bool showInspector      = true;
		bool showAssets         = true;
		bool showMaterialEditor = true;
		bool showAnimation      = false;
		bool showAudioDebug     = false;
		bool showGBufferDebug   = false;
		bool showModelDebug     = false;
		bool showSettings       = false;
		bool showShortcuts      = false;

		bool  snapEnabled    = false;
		float snapTranslate  = 0.5f;
		float snapRotateDeg  = 15.0f;
		float snapScale      = 0.1f;

		bool  showGrid             = true;
		bool  lockAspect           = false;
		float lockedAspect         = 16.0f / 9.0f;

		int theme = 2;

		char hierarchyFilter[128] = {};
		char assetFilter[128]     = {};

		std::unordered_set<std::string> lockedGuids;

		bool   pendingConfirmNew      = false;
		bool   pendingConfirmOpen     = false;
		bool   pendingConfirmDelete   = false;
		Entity pendingDeleteEntity;

		std::string pendingOpenPath;

		void MarkDirty();
		void ClearDirty();

		void NewScene();
		void OpenSceneDialog();
		void SaveScene();
		void SaveSceneAs();
		bool LoadSceneFromPath(const std::string& path);

		void Play();
		void Pause();
		void Stop();
		void Step();

		void HandleShortcuts();
		void DrawShortcutsOverlay();
		void DrawSettingsWindow();
		void DrawConfirmModals();
		void UpdateWindowTitle() const;

		bool CanUseEditorShortcuts() const;
		bool IsEntityLocked(Entity entity) const;
		void ToggleEntityLocked(Entity entity);

		static bool MatchesFilter(const std::string& text, const char* filter);
	};

	EditorSession& GetEditor();
} // namespace Engine::UI
