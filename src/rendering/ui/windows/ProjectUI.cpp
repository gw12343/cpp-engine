#ifndef GAME_BUILD

#include "ProjectUI.h"

#include "core/EditorPrefs.h"
#include "core/EngineData.h"
#include "core/ProjectSettings.h"
#include "core/Window.h"
#include "rendering/ui/EditorSession.h"
#include "rendering/ui/IconsFontAwesome6.h"

#include "imgui.h"

#include <nfd.h>
#include <cstdio>
#include <filesystem>
#include <string>

namespace Engine::UI {
	namespace fs = std::filesystem;

	namespace {
		bool        g_showSettings   = false;
		bool        g_showNewProject = false;
		char        g_newName[128]   = "New Project";
		std::string g_newFolder;

		std::string PickFolder()
		{
			nfdchar_t*  folder = nullptr;
			nfdresult_t result = NFD_PickFolder(nullptr, &folder);
			if (result != NFD_OKAY || !folder) {
				return {};
			}
			std::string path(folder);
			free(folder);
			return path;
		}

		std::string PickSceneFile()
		{
			const std::string dir = GetProject().IsOpen() ? GetProject().ScenesDirectory() : std::string{};
			nfdchar_t*        out = nullptr;
			nfdresult_t       res = NFD_OpenDialog("json", dir.empty() ? nullptr : dir.c_str(), &out);
			if (res != NFD_OKAY || !out) {
				return {};
			}
			std::string path(out);
			free(out);
			return path;
		}

		void LoadProjectEditorScene()
		{
			auto& project = GetProject();
			if (!project.IsOpen()) {
				return;
			}
			std::string scene = project.lastEditorScene;
			if (scene.empty() || !fs::exists(scene)) {
				scene = project.SourceScenePath(0);
			}
			if (!scene.empty() && fs::exists(scene)) {
				GetEditor().LoadSceneFromPath(scene);
			}
		}
	} // namespace

	bool OpenProjectFolder(const std::string& folder)
	{
		if (!GetProject().Open(folder)) {
			return false;
		}
		GetEditorPrefs().RememberProject(GetProject().Root());
		LoadProjectEditorScene();
		GetEditor().UpdateWindowTitle();
		return true;
	}

	bool CreateProjectFolder(const std::string& folder, const std::string& name)
	{
		if (!GetProject().Create(folder, name)) {
			return false;
		}
		GetEditorPrefs().RememberProject(GetProject().Root());
		GetEditor().UpdateWindowTitle();
		return true;
	}

	void DrawProjectMenu()
	{
		if (!ImGui::BeginMenu("Project")) {
			return;
		}
		if (ImGui::MenuItem("New Project...")) {
			g_showNewProject = true;
			g_newFolder.clear();
			std::snprintf(g_newName, sizeof(g_newName), "New Project");
		}
		if (ImGui::MenuItem("Open Project...")) {
			const std::string folder = PickFolder();
			if (!folder.empty()) {
				OpenProjectFolder(folder);
			}
		}
		if (ImGui::BeginMenu("Recent Projects", !GetEditorPrefs().recentProjects.empty())) {
			for (const auto& recent : GetEditorPrefs().recentProjects) {
				if (ImGui::MenuItem(recent.c_str())) {
					OpenProjectFolder(recent);
				}
			}
			ImGui::EndMenu();
		}
		ImGui::Separator();
		if (ImGui::MenuItem("Save Project", nullptr, false, GetProject().IsOpen())) {
			GetProject().Save();
		}
		if (ImGui::MenuItem("Project Settings", nullptr, false, GetProject().IsOpen())) {
			g_showSettings = true;
		}
		ImGui::EndMenu();
	}

	static void DrawLauncher()
	{
		ImGui::OpenPopup("Open Project");
		const ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		if (!ImGui::BeginPopupModal("Open Project", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			return;
		}

		ImGui::TextUnformatted("Select a project folder to continue.");
		ImGui::Separator();

		if (ImGui::Button("Open Project...", ImVec2(280, 0))) {
			const std::string folder = PickFolder();
			if (!folder.empty() && OpenProjectFolder(folder)) {
				ImGui::CloseCurrentPopup();
			}
		}
		if (ImGui::Button("New Project...", ImVec2(280, 0))) {
			g_showNewProject = true;
			g_newFolder.clear();
		}

		if (!GetEditorPrefs().recentProjects.empty()) {
			ImGui::Separator();
			ImGui::TextUnformatted("Recent");
			for (const auto& recent : GetEditorPrefs().recentProjects) {
				if (ImGui::Selectable(recent.c_str())) {
					if (OpenProjectFolder(recent)) {
						ImGui::CloseCurrentPopup();
					}
				}
			}
		}

		ImGui::EndPopup();
	}

	static void DrawNewProjectPopup()
	{
		if (g_showNewProject) {
			ImGui::OpenPopup("New Project");
			g_showNewProject = false;
		}
		if (!ImGui::BeginPopupModal("New Project", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			return;
		}

		ImGui::InputText("Name", g_newName, sizeof(g_newName));
		ImGui::Text("Folder: %s", g_newFolder.empty() ? "(none selected)" : g_newFolder.c_str());
		if (ImGui::Button("Choose Folder...")) {
			g_newFolder = PickFolder();
		}
		ImGui::Separator();
		const bool canCreate = !g_newFolder.empty() && g_newName[0] != '\0';
		if (!canCreate) {
			ImGui::BeginDisabled();
		}
		if (ImGui::Button("Create", ImVec2(120, 0))) {
			if (CreateProjectFolder(g_newFolder, g_newName)) {
				ImGui::CloseCurrentPopup();
			}
		}
		if (!canCreate) {
			ImGui::EndDisabled();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0))) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	static void DrawSettings()
	{
		if (!g_showSettings || !GetProject().IsOpen()) {
			return;
		}
		if (!ImGui::Begin("Project Settings", &g_showSettings)) {
			ImGui::End();
			return;
		}

		auto& project      = GetProject();
		bool  dirtyProject = false;

		char nameBuf[128];
		std::snprintf(nameBuf, sizeof(nameBuf), "%s", project.name.c_str());
		if (ImGui::InputText("Game Name", nameBuf, sizeof(nameBuf))) {
			project.name = nameBuf;
			dirtyProject = true;
		}
		ImGui::TextDisabled("%s", project.Root().c_str());

		ImGui::Separator();
		ImGui::TextUnformatted("Game Window");
		dirtyProject |= ImGui::DragInt("Width", &project.windowWidth, 1, 320, 7680);
		dirtyProject |= ImGui::DragInt("Height", &project.windowHeight, 1, 240, 4320);
		const char* modes[] = {"Windowed", "Fullscreen", "Borderless"};
		int         mode    = static_cast<int>(project.windowMode);
		if (ImGui::Combo("Mode", &mode, modes, IM_ARRAYSIZE(modes))) {
			project.windowMode = static_cast<WindowMode>(mode);
			dirtyProject       = true;
		}
		dirtyProject |= ImGui::Checkbox("VSync", &project.vsync);

		ImGui::Separator();
		ImGui::TextUnformatted("Build Scenes");
		ImGui::TextDisabled("Scene 0 starts the game build. Scripts call loadScene(index).");

		if (ImGui::BeginTable("ProjectScenes", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
			ImGui::TableSetupColumn("Index", ImGuiTableColumnFlags_WidthFixed, 48.0f);
			ImGui::TableSetupColumn("Path", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("Move", ImGuiTableColumnFlags_WidthFixed, 70.0f);
			ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 28.0f);
			ImGui::TableHeadersRow();

			int removeAt = -1;
			for (int i = 0; i < project.SceneCount(); ++i) {
				ImGui::PushID(i);
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				if (i == 0) {
					ImGui::TextUnformatted("0 *");
				}
				else {
					ImGui::Text("%d", i);
				}
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(project.scenes[static_cast<size_t>(i)].c_str());
				ImGui::TableNextColumn();
				if (ImGui::SmallButton("^") && i > 0) {
					project.MoveScene(i, -1);
					dirtyProject = true;
				}
				ImGui::SameLine();
				if (ImGui::SmallButton("v") && i + 1 < project.SceneCount()) {
					project.MoveScene(i, 1);
					dirtyProject = true;
				}
				ImGui::TableNextColumn();
				if (ImGui::SmallButton("X")) {
					removeAt = i;
				}
				ImGui::PopID();
			}
			ImGui::EndTable();
			if (removeAt >= 0) {
				project.RemoveScene(removeAt);
				dirtyProject = true;
			}
		}

		if (ImGui::Button("Add Current Scene")) {
			if (!GetEditor().scenePath.empty()) {
				project.AddScene(GetEditor().scenePath);
				dirtyProject = true;
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Add Scene...")) {
			const std::string path = PickSceneFile();
			if (!path.empty()) {
				project.AddScene(path);
				dirtyProject = true;
			}
		}

		if (dirtyProject) {
			project.Save();
		}
		ImGui::End();
	}

	void DrawProjectWindows()
	{
		if (!GetProject().IsOpen()) {
			DrawLauncher();
		}
		DrawNewProjectPopup();
		DrawSettings();
	}

} // namespace Engine::UI

#endif
