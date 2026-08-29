#include "EditorPrefs.h"

#include "core/EnginePaths.h"
#include "utils/Logger.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

namespace Engine {
	namespace fs = std::filesystem;

	EditorPrefs& EditorPrefs::Get()
	{
		static EditorPrefs instance;
		return instance;
	}

	std::string EditorPrefs::FilePath()
	{
		return (std::filesystem::path(GetEnginePaths().EngineRoot()) / "editor.json").generic_string();
	}

	void EditorPrefs::RememberProject(const std::string& folder)
	{
		if (folder.empty()) {
			return;
		}
		std::error_code ec;
		lastProject = fs::weakly_canonical(folder, ec).generic_string();
		if (lastProject.empty()) {
			lastProject = fs::absolute(folder, ec).generic_string();
		}

		recentProjects.erase(std::remove(recentProjects.begin(), recentProjects.end(), lastProject), recentProjects.end());
		recentProjects.insert(recentProjects.begin(), lastProject);
		if (recentProjects.size() > 12) {
			recentProjects.resize(12);
		}
		Save();
	}

	bool EditorPrefs::Load()
	{
		const std::string path = FilePath();
		std::ifstream     in(path);
		if (!in) {
			return false;
		}
		try {
			nlohmann::json j;
			in >> j;
			lastProject   = j.value("lastProject", lastProject);
			editorWidth   = j.value("editorWidth", editorWidth);
			editorHeight  = j.value("editorHeight", editorHeight);
			recentProjects.clear();
			if (j.contains("recentProjects") && j["recentProjects"].is_array()) {
				for (const auto& entry : j["recentProjects"]) {
					if (entry.is_string()) {
						recentProjects.push_back(entry.get<std::string>());
					}
				}
			}
			return true;
		}
		catch (const std::exception& e) {
			if (auto log = Logger::get("core")) {
				log->error("Failed to parse editor prefs {}: {}", path, e.what());
			}
			return false;
		}
	}

	bool EditorPrefs::Save() const
	{
		nlohmann::json j;
		j["lastProject"]    = lastProject;
		j["recentProjects"] = recentProjects;
		j["editorWidth"]    = editorWidth;
		j["editorHeight"]   = editorHeight;

		const std::string path = FilePath();
		std::ofstream     out(path, std::ios::trunc);
		if (!out) {
			return false;
		}
		out << j.dump(4) << '\n';
		return true;
	}

} // namespace Engine
