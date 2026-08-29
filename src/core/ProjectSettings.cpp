#include "ProjectSettings.h"

#include "core/EngineData.h"
#include "utils/Logger.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

namespace Engine {
	namespace fs = std::filesystem;

	ProjectSettings& ProjectSettings::Get()
	{
		static ProjectSettings instance;
		return instance;
	}

	const char* ProjectSettings::WindowModeName(WindowMode mode)
	{
		switch (mode) {
			case WindowMode::Fullscreen: return "fullscreen";
			case WindowMode::Borderless: return "borderless";
			case WindowMode::Windowed:
			default: return "windowed";
		}
	}

	WindowMode ProjectSettings::WindowModeFromName(const std::string& name)
	{
		std::string lower = name;
		std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		if (lower == "fullscreen") return WindowMode::Fullscreen;
		if (lower == "borderless") return WindowMode::Borderless;
		return WindowMode::Windowed;
	}

	bool ProjectSettings::IsProjectDirectory(const std::string& folder)
	{
		if (folder.empty()) {
			return false;
		}
		std::error_code ec;
		return fs::exists(fs::path(folder) / FileName(), ec);
	}

	std::string ProjectSettings::FilePath() const
	{
		if (m_root.empty()) {
			return FileName();
		}
		return (fs::path(m_root) / FileName()).generic_string();
	}

	std::string ProjectSettings::ScenesDirectory() const
	{
		if (m_root.empty()) {
			return "scenes";
		}
		return (fs::path(m_root) / "scenes").generic_string();
	}

	std::string ProjectSettings::AssetsDirectory() const
	{
		if (m_root.empty()) {
			return "assets";
		}
		return (fs::path(m_root) / "assets").generic_string();
	}

	std::string ProjectSettings::ScriptsDirectory() const
	{
		if (m_root.empty()) {
			return "scripts";
		}
		return (fs::path(m_root) / "scripts").generic_string();
	}

	std::string ProjectSettings::NormalizePath(const std::string& path) const
	{
		if (path.empty()) {
			return {};
		}
		std::error_code ec;
		fs::path        abs = fs::absolute(path, ec);
		if (ec) {
			return fs::path(path).generic_string();
		}
		const fs::path root = m_root.empty() ? fs::current_path() : fs::path(m_root);
		fs::path       rel  = fs::relative(abs, root, ec);
		if (ec) {
			return abs.generic_string();
		}
		return rel.generic_string();
	}

	std::string ProjectSettings::SourceScenePath(int index) const
	{
		if (index < 0 || index >= SceneCount()) {
			return {};
		}
		return scenes[static_cast<size_t>(index)];
	}

	std::string ProjectSettings::RuntimeScenePath(int index) const
	{
		std::string path = SourceScenePath(index);
		if (path.empty()) {
			return {};
		}
#ifdef GAME_BUILD
		fs::path p(path);
		if (p.extension() == ".json") {
			p.replace_extension(".bin");
		}
		return p.generic_string();
#else
		return path;
#endif
	}

	int ProjectSettings::FindSceneIndex(const std::string& path) const
	{
		const std::string norm = NormalizePath(path);
		for (int i = 0; i < SceneCount(); ++i) {
			if (NormalizePath(scenes[static_cast<size_t>(i)]) == norm) {
				return i;
			}
			fs::path a(scenes[static_cast<size_t>(i)]);
			fs::path b(path);
			if (a.stem() == b.stem() && a.parent_path() == b.parent_path()) {
				return i;
			}
		}
		return -1;
	}

	void ProjectSettings::AddScene(const std::string& path)
	{
		const std::string norm = NormalizePath(path);
		if (norm.empty() || FindSceneIndex(norm) >= 0) {
			return;
		}
		scenes.push_back(norm);
	}

	void ProjectSettings::RemoveScene(int index)
	{
		if (index < 0 || index >= SceneCount()) {
			return;
		}
		scenes.erase(scenes.begin() + index);
	}

	void ProjectSettings::MoveScene(int index, int delta)
	{
		const int dest = index + delta;
		if (index < 0 || dest < 0 || index >= SceneCount() || dest >= SceneCount()) {
			return;
		}
		std::swap(scenes[static_cast<size_t>(index)], scenes[static_cast<size_t>(dest)]);
	}

	bool ProjectSettings::LoadFile()
	{
		std::ifstream in(FilePath());
		if (!in) {
			return false;
		}
		try {
			nlohmann::json j;
			in >> j;
			name            = j.value("name", name);
			lastEditorScene = j.value("lastEditorScene", std::string{});
			if (j.contains("window") && j["window"].is_object()) {
				const auto& w = j["window"];
				windowWidth   = w.value("width", windowWidth);
				windowHeight  = w.value("height", windowHeight);
				vsync         = w.value("vsync", vsync);
				if (w.contains("mode")) {
					windowMode = WindowModeFromName(w["mode"].get<std::string>());
				}
			}
			scenes.clear();
			if (j.contains("scenes") && j["scenes"].is_array()) {
				for (const auto& entry : j["scenes"]) {
					if (entry.is_string()) {
						AddScene(entry.get<std::string>());
					}
				}
			}
			return true;
		}
		catch (const std::exception& e) {
			GetDefaultLogger()->error("Failed to parse {}: {}", FilePath(), e.what());
			return false;
		}
	}

	bool ProjectSettings::Open(const std::string& folder)
	{
		if (!IsProjectDirectory(folder)) {
			GetDefaultLogger()->error("Not a project folder (missing {}): {}", FileName(), folder);
			return false;
		}

		std::error_code ec;
		m_root = fs::weakly_canonical(folder, ec).generic_string();
		if (m_root.empty()) {
			m_root = fs::absolute(folder, ec).generic_string();
		}

		name            = "Game";
		lastEditorScene.clear();
		windowWidth     = 1600;
		windowHeight    = 900;
		windowMode      = WindowMode::Windowed;
		vsync           = true;
		scenes.clear();

		if (!LoadFile()) {
			m_open = false;
			return false;
		}
		m_open = true;
		GetDefaultLogger()->info("Opened project '{}' at {} ({} scene(s))", name, m_root, scenes.size());
		return true;
	}

	bool ProjectSettings::Create(const std::string& folder, const std::string& projectName)
	{
		if (folder.empty()) {
			return false;
		}
		std::error_code ec;
		fs::create_directories(fs::path(folder) / "scenes", ec);
		fs::create_directories(fs::path(folder) / "assets", ec);
		fs::create_directories(fs::path(folder) / "scripts", ec);

		m_root          = fs::weakly_canonical(folder, ec).generic_string();
		if (m_root.empty()) {
			m_root = fs::absolute(folder, ec).generic_string();
		}
		name            = projectName.empty() ? fs::path(m_root).filename().string() : projectName;
		if (name.empty()) {
			name = "Game";
		}
		lastEditorScene.clear();
		windowWidth  = 1600;
		windowHeight = 900;
		windowMode   = WindowMode::Windowed;
		vsync        = true;
		scenes.clear();
		m_open = true;

		if (!Save()) {
			m_open = false;
			return false;
		}
		return Open(m_root);
	}

	bool ProjectSettings::Save() const
	{
		if (!m_open && m_root.empty()) {
			GetDefaultLogger()->error("Cannot save project: no project is open");
			return false;
		}
		nlohmann::json j;
		j["name"]            = name;
		j["lastEditorScene"] = lastEditorScene;
		j["window"]          = {
            {"width", windowWidth},
            {"height", windowHeight},
            {"mode", WindowModeName(windowMode)},
            {"vsync", vsync},
        };
		j["scenes"] = scenes;

		std::ofstream out(FilePath(), std::ios::trunc);
		if (!out) {
			GetDefaultLogger()->error("Failed to write {}", FilePath());
			return false;
		}
		out << j.dump(4) << '\n';
		return true;
	}

	void ProjectSettings::Close()
	{
		m_open = false;
		m_root.clear();
		scenes.clear();
		lastEditorScene.clear();
	}

} // namespace Engine
