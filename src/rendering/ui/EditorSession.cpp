#include "EditorSession.h"

#include "UIManager.h"
#include "imgui.h"
#include "windows/SceneViewWindow.h"

#include "assets/impl/JSONSceneLoader.h"
#include "assets/impl/PrefabLoader.h"
#include "assets/Prefab.h"
#include "components/AllComponents.h"
#include "components/impl/EntityMetadataComponent.h"
#include "core/EngineData.h"
#include "core/Input.h"
#include "core/SceneManager.h"
#include "core/Window.h"
#include "core/module/ModuleManager.h"
#include "physics/PhysicsManager.h"
#include "rendering/particles/ParticleManager.h"
#include "rendering/ui/GameUIManager.h"
#include "rendering/ui/IconsFontAwesome6.h"
#include "scripting/ScriptManager.h"

#include <nfd.h>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <filesystem>

#ifndef GAME_BUILD

namespace fs = std::filesystem;

namespace Engine::UI {

	EditorSession& GetEditor()
	{
		static EditorSession session;
		return session;
	}

	static std::string PickJsonFile(bool save)
	{
		nfdchar_t*  outPath = nullptr;
		nfdresult_t result  = save ? NFD_SaveDialog("json", "scenes", &outPath) : NFD_OpenDialog("json", "scenes", &outPath);
		if (result != NFD_OKAY || !outPath) {
			return {};
		}
		std::string path(outPath);
		free(outPath);
		if (save) {
			const bool hasJson = path.size() >= 5 && path.compare(path.size() - 5, 5, ".json") == 0;
			if (!hasJson) {
				path += ".json";
			}
		}
		return path;
	}

	static std::string PrefabDialogFolder()
	{
		std::error_code ec;
		fs::path        dir = fs::absolute("assets/prefabs", ec);
		if (ec) {
			dir = fs::current_path() / "assets" / "prefabs";
		}
		fs::create_directories(dir, ec);
		return dir.lexically_normal().make_preferred().string();
	}

	static std::string PickPrefabFile(bool save)
	{
		const std::string defaultDir = PrefabDialogFolder();
		nfdchar_t*        outPath    = nullptr;
		nfdresult_t       result     = save ? NFD_SaveDialog("prefab", defaultDir.c_str(), &outPath)
		                                    : NFD_OpenDialog("prefab", defaultDir.c_str(), &outPath);
		if (result == NFD_CANCEL) {
			return {};
		}
		if (result != NFD_OKAY || !outPath) {
			const char* err = NFD_GetError();
			GetDefaultLogger()->error("Prefab file dialog failed: {}", err ? err : "unknown NFD error");
			return {};
		}
		std::string path(outPath);
		free(outPath);
		if (save) {
			const bool hasExt = path.size() >= 7 && path.compare(path.size() - 7, 7, ".prefab") == 0;
			if (!hasExt) {
				path += ".prefab";
			}
		}
		return path;
	}

	static void ClearPhysicsBodies()
	{
		auto&        physics = GetPhysics();
		BodyIDVector outBodies;
		physics.GetPhysicsSystem()->GetBodies(outBodies);
		for (auto body : outBodies) {
			if (physics.GetPhysicsSystem()->GetBodyInterface().IsAdded(body)) {
				physics.GetPhysicsSystem()->GetBodyInterface().RemoveBody(body);
			}
		}
	}

	static void UnloadActiveScene()
	{
		GetUI().m_selectedEntity = Entity();
		GetParticleManager().ResetInternalManager();
		GetScriptManager().GetEventBus().ClearAllSubscriptions();
		ClearPhysicsBodies();
		// Close Rml docs before destroying the registry — resetDocuments() must
		// not run here because GetCurrentScene() is null after Unload.
		GetGameUIManager().CloseAllDocuments();
		GetAssetManager().Unload<Scene>(GetSceneManager().GetActiveScene());
	}

	void EditorSession::MarkDirty()
	{
		if (GetState() == EDITOR) {
			dirty = true;
			UpdateWindowTitle();
		}
	}

	void EditorSession::ClearDirty()
	{
		dirty = false;
		UpdateWindowTitle();
	}

	bool EditorSession::LoadSceneFromPath(const std::string& path)
	{
		if (path.empty() || !fs::exists(path)) {
			GetDefaultLogger()->error("Scene does not exist: {}", path);
			return false;
		}

		UnloadActiveScene();
		SetState(EDITOR);
		GetSceneManager().SetActiveScene(GetAssetManager().Load<Scene>(path));
		GetGameUIManager().resetDocuments();
		scenePath         = path;
		playSnapshotValid = false;
		ClearDirty();
		return true;
	}

	void EditorSession::NewScene()
	{
		if (GetState() != EDITOR) {
			return;
		}
		if (dirty) {
			pendingConfirmNew = true;
			return;
		}

		fs::create_directories("scenes");
		const std::string untitled = "scenes/untitled.json";
		{
			std::ofstream os(untitled);
			os << "{\n    \"entities\": []\n}\n";
		}
		LoadSceneFromPath(untitled);
		dirty = true;
		UpdateWindowTitle();
	}

	void EditorSession::OpenSceneDialog()
	{
		if (GetState() != EDITOR) {
			return;
		}
		std::string path = PickJsonFile(false);
		if (path.empty()) {
			return;
		}
		if (dirty) {
			pendingConfirmOpen = true;
			pendingOpenPath    = path;
			return;
		}
		LoadSceneFromPath(path);
	}

	void EditorSession::SaveScene()
	{
		if (GetState() != EDITOR) {
			return;
		}
		if (scenePath.empty()) {
			SaveSceneAs();
			return;
		}
		SCENE_LOADER::SerializeScene(GetSceneManager().GetActiveScene(), scenePath);
		ClearDirty();
		GetDefaultLogger()->info("Saved scene: {}", scenePath);
	}

	void EditorSession::SaveSceneAs()
	{
		if (GetState() != EDITOR) {
			return;
		}
		std::string path = PickJsonFile(true);
		if (path.empty()) {
			return;
		}
		scenePath = path;
		SaveScene();
	}

	bool EditorSession::SaveEntityAsPrefab(Entity root)
	{
		if (!root || !root.IsValid()) {
			return false;
		}
		std::string path = PickPrefabFile(true);
		if (path.empty()) {
			return false;
		}

		Prefab prefab;
		if (!Prefab::CaptureFromEntity(root, prefab)) {
			GetDefaultLogger()->error("Failed to capture prefab from '{}'", root.GetName());
			return false;
		}
		if (prefab.m_name.empty()) {
			prefab.m_name = fs::path(path).stem().string();
		}
		if (!PrefabLoader::SaveToFile(prefab, path)) {
			return false;
		}
		GetAssetManager().Load<Prefab>(path);
		GetDefaultLogger()->info("Saved prefab: {}", path);
		return true;
	}

	Entity EditorSession::InstantiatePrefabDialog(const EntityHandle& parent)
	{
		std::string path = PickPrefabFile(false);
		if (path.empty()) {
			return {};
		}
		PrefabHandle handle = GetAssetManager().Load<Prefab>(path);
		Entity       root   = InstantiatePrefab(handle, parent);
		if (root && root.IsValid()) {
			MarkDirty();
		}
		return root;
	}

	void EditorSession::Play()
	{
		if (GetState() == PLAYING) {
			return;
		}

		GetCamera().SaveEditorLocation();
		const bool wasPaused = GetState() == PAUSED;
		if (GetState() == EDITOR) {
			fs::create_directories("scenes");
			SCENE_LOADER::SerializeScene(GetSceneManager().GetActiveScene(), playSnapshotPath);
			playSnapshotValid = true;
		}
		SetState(PLAYING);

		if (!wasPaused) {
			Get().manager->StartGame();
			GetGameUIManager().resetDocuments();
		}
	}

	void EditorSession::Pause()
	{
		if (GetState() == PLAYING) {
			SetState(PAUSED);
		}
	}

	void EditorSession::Stop()
	{
		if (GetState() == EDITOR) {
			return;
		}

		GetCamera().LoadEditorLocation();
		UnloadActiveScene();
		SetState(EDITOR);

		const std::string reload = (playSnapshotValid && fs::exists(playSnapshotPath)) ? playSnapshotPath : scenePath;
		GetSceneManager().SetActiveScene(GetAssetManager().Load<Scene>(reload));
		GetGameUIManager().resetDocuments();
		UpdateWindowTitle();
	}

	void EditorSession::Step()
	{
		if (GetState() == PLAYING) {
			Pause();
		}
		if (GetState() == PAUSED) {
			Get().stepOneFrame = true;
		}
	}

	bool EditorSession::CanUseEditorShortcuts() const
	{
		if (ImGui::GetIO().WantTextInput) {
			return false;
		}
		return GetState() != PLAYING;
	}

	void EditorSession::HandleShortcuts()
	{
		const bool ctrl = ImGui::GetIO().KeyCtrl || ImGui::GetIO().KeySuper;
		const bool shift = ImGui::GetIO().KeyShift;

		if (ImGui::IsKeyPressed(ImGuiKey_F1, false) && !ImGui::GetIO().WantTextInput) {
			showShortcuts = !showShortcuts;
		}
		if (ctrl && ImGui::IsKeyPressed(ImGuiKey_N, false) && !ImGui::GetIO().WantTextInput) {
			NewScene();
		}
		if (ctrl && ImGui::IsKeyPressed(ImGuiKey_O, false) && !ImGui::GetIO().WantTextInput) {
			OpenSceneDialog();
		}
		if (ctrl && ImGui::IsKeyPressed(ImGuiKey_S, false) && !ImGui::GetIO().WantTextInput) {
			SaveScene();
		}
		if (ImGui::IsKeyPressed(ImGuiKey_F5, false) && !ImGui::GetIO().WantTextInput) {
			if (shift) {
				Stop();
			}
			else {
				Play();
			}
		}
		if (ctrl && ImGui::IsKeyPressed(ImGuiKey_P, false) && !ImGui::GetIO().WantTextInput) {
			Play();
		}
		if (ImGui::IsKeyPressed(ImGuiKey_F10, false) && !ImGui::GetIO().WantTextInput) {
			if (GetState() == PLAYING) {
				Pause();
			}
			else if (GetState() == PAUSED) {
				Step();
			}
		}

		if (!CanUseEditorShortcuts()) {
			return;
		}

		const bool flying = GetInput().IsMousePressed(GLFW_MOUSE_BUTTON_RIGHT);
		if (!flying) {
			if (ImGui::IsKeyPressed(ImGuiKey_W, false)) {
				SceneViewWindow::mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
			}
			if (ImGui::IsKeyPressed(ImGuiKey_E, false)) {
				SceneViewWindow::mCurrentGizmoOperation = ImGuizmo::ROTATE;
			}
			if (ImGui::IsKeyPressed(ImGuiKey_R, false)) {
				SceneViewWindow::mCurrentGizmoOperation = ImGuizmo::SCALE;
			}
			if (ImGui::IsKeyPressed(ImGuiKey_T, false)) {
				SceneViewWindow::mCurrentGizmoOperation = ImGuizmo::BOUNDS;
			}
			if (ImGui::IsKeyPressed(ImGuiKey_Q, false)) {
				SceneViewWindow::mCurrentGizmoMode = ImGuizmo::LOCAL;
			}
			if (ImGui::IsKeyPressed(ImGuiKey_X, false)) {
				SceneViewWindow::mCurrentGizmoMode = ImGuizmo::WORLD;
			}
		}

		if (ImGui::IsKeyPressed(ImGuiKey_Delete, false)) {
			Entity selected = GetUI().m_selectedEntity;
			if (selected && selected.IsValid()) {
				pendingDeleteEntity  = selected;
				pendingConfirmDelete = true;
			}
		}

		// RMB fly uses Ctrl as speed and D as strafe — never duplicate while flying.
		if (ctrl && !flying && ImGui::IsKeyPressed(ImGuiKey_D, false)) {
			Entity selected = GetUI().m_selectedEntity;
			if (selected && selected.IsValid()) {
				GetUI().m_selectedEntity = GetUI().DuplicateEntity(selected);
				MarkDirty();
			}
		}
	}

	void EditorSession::DrawShortcutsOverlay()
	{
		if (!showShortcuts) {
			return;
		}
		ImGui::SetNextWindowSize(ImVec2(420, 0), ImGuiCond_FirstUseEver);
		if (!ImGui::Begin("Keyboard Shortcuts", &showShortcuts)) {
			ImGui::End();
			return;
		}
		ImGui::TextUnformatted("W / E / R / T     Translate / Rotate / Scale / Bounds");
		ImGui::TextUnformatted("Q / X             Local / World gizmo");
		ImGui::TextUnformatted("F                 Frame selection");
		ImGui::TextUnformatted("Ctrl+D            Duplicate");
		ImGui::TextUnformatted("Delete            Delete selected (confirms)");
		ImGui::TextUnformatted("Ctrl+S            Save scene");
		ImGui::TextUnformatted("F5 / Ctrl+P       Play");
		ImGui::TextUnformatted("Shift+F5          Stop");
		ImGui::TextUnformatted("F10               Pause / Step");
		ImGui::TextUnformatted("RMB + WASD        Fly camera");
		ImGui::TextUnformatted("Alt + LMB         Orbit selection");
		ImGui::TextUnformatted("Scroll            Dolly camera");
		ImGui::TextUnformatted("Ctrl (hold)       Snap while dragging gizmo");
		ImGui::End();
	}

	void EditorSession::DrawSettingsWindow()
	{
		if (!showSettings) {
			return;
		}
		if (!ImGui::Begin("Editor Settings", &showSettings)) {
			ImGui::End();
			return;
		}

		const char* themes[] = {"Play Dark", "Catppuccin", "Editor Gray", "Minimal", "Olive"};
		ImGui::Combo("Theme", &theme, themes, IM_ARRAYSIZE(themes));

		ImGui::Separator();
		ImGui::TextUnformatted("Camera");
		ImGui::SliderFloat("Move Speed", &GetCamera().m_movementSpeed, 0.5f, 40.0f);
		ImGui::SliderFloat("Mouse Sensitivity", &GetCamera().m_mouseSensitivity, 0.01f, 0.5f);
		ImGui::SliderFloat("Field of View", &GetCamera().m_fov, 30.0f, 120.0f);

		ImGui::Separator();
		ImGui::TextUnformatted("Viewport");
		ImGui::Checkbox("Show Grid", &showGrid);
		ImGui::Checkbox("Lock 16:9 Aspect", &lockAspect);

		ImGui::Separator();
		ImGui::TextUnformatted("Snap");
		ImGui::Checkbox("Snap Enabled", &snapEnabled);
		ImGui::DragFloat("Translate", &snapTranslate, 0.05f, 0.01f, 10.0f);
		ImGui::DragFloat("Rotate (deg)", &snapRotateDeg, 1.0f, 1.0f, 90.0f);
		ImGui::DragFloat("Scale", &snapScale, 0.01f, 0.01f, 2.0f);

		ImGui::Separator();
		ImGui::TextUnformatted("Bloom");
		ImGui::SliderFloat("Threshold", &GetRenderSettings()->bloom_threshold, 0.1f, 2.0f);
		ImGui::SliderFloat("Knee", &GetRenderSettings()->bloom_knee, 0.1f, 0.5f);

		ImGui::End();
	}

	void EditorSession::DrawConfirmModals()
	{
		if (pendingConfirmNew) {
			ImGui::OpenPopup("Discard Changes##new");
			pendingConfirmNew = false;
		}
		if (pendingConfirmOpen) {
			ImGui::OpenPopup("Discard Changes##open");
			pendingConfirmOpen = false;
		}
		if (pendingConfirmDelete) {
			ImGui::OpenPopup("Delete Entity##confirm");
		}

		if (ImGui::BeginPopupModal("Discard Changes##new", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::TextUnformatted("Discard unsaved changes and create a new scene?");
			if (ImGui::Button("Discard", ImVec2(120, 0))) {
				dirty = false;
				ImGui::CloseCurrentPopup();
				NewScene();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(120, 0))) {
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		if (ImGui::BeginPopupModal("Discard Changes##open", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::TextUnformatted("Discard unsaved changes and open another scene?");
			if (ImGui::Button("Discard", ImVec2(120, 0))) {
				dirty = false;
				std::string path = pendingOpenPath;
				pendingOpenPath.clear();
				ImGui::CloseCurrentPopup();
				LoadSceneFromPath(path);
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(120, 0))) {
				pendingOpenPath.clear();
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		if (ImGui::BeginPopupModal("Delete Entity##confirm", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			const char* name = pendingDeleteEntity.IsValid() ? pendingDeleteEntity.GetName().c_str() : "Entity";
			ImGui::Text("Delete '%s' and its children?", name);
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
			if (ImGui::Button("Delete", ImVec2(120, 0))) {
				if (pendingDeleteEntity.IsValid()) {
					if (GetUI().m_selectedEntity == pendingDeleteEntity) {
						GetUI().m_selectedEntity = Entity();
					}
					pendingDeleteEntity.Destroy();
					MarkDirty();
				}
				pendingConfirmDelete = false;
				pendingDeleteEntity  = Entity();
				ImGui::CloseCurrentPopup();
			}
			ImGui::PopStyleColor();
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(120, 0))) {
				pendingConfirmDelete = false;
				pendingDeleteEntity  = Entity();
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
	}

	void EditorSession::UpdateWindowTitle() const
	{
		std::string name = scenePath.empty() ? "Untitled" : fs::path(scenePath).filename().string();
		std::string title = std::string("cpp-engine - ") + (dirty ? "*" : "") + name;
		if (GetState() == PLAYING) {
			title += "  [PLAYING]";
		}
		else if (GetState() == PAUSED) {
			title += "  [PAUSED]";
		}
		glfwSetWindowTitle(GetWindow().GetNativeWindow(), title.c_str());
	}

	bool EditorSession::IsEntityLocked(Entity entity) const
	{
		if (!entity || !entity.HasComponent<Components::EntityMetadata>()) {
			return false;
		}
		return lockedGuids.count(entity.GetComponent<Components::EntityMetadata>().guid) > 0;
	}

	void EditorSession::ToggleEntityLocked(Entity entity)
	{
		if (!entity || !entity.HasComponent<Components::EntityMetadata>()) {
			return;
		}
		const std::string& guid = entity.GetComponent<Components::EntityMetadata>().guid;
		if (lockedGuids.count(guid)) {
			lockedGuids.erase(guid);
		}
		else {
			lockedGuids.insert(guid);
		}
	}

	bool EditorSession::MatchesFilter(const std::string& text, const char* filter)
	{
		if (!filter || filter[0] == '\0') {
			return true;
		}
		std::string a = text;
		std::string b = filter;
		std::transform(a.begin(), a.end(), a.begin(), [](unsigned char c) { return (char) std::tolower(c); });
		std::transform(b.begin(), b.end(), b.begin(), [](unsigned char c) { return (char) std::tolower(c); });
		return a.find(b) != std::string::npos;
	}

} // namespace Engine::UI

#endif
