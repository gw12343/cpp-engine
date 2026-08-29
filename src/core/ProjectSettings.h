#pragma once

#include <string>
#include <vector>

namespace Engine {

	enum class WindowMode { Windowed = 0, Fullscreen = 1, Borderless = 2 };

	class ProjectSettings {
	  public:
		std::string              name = "Game";
		std::string              lastEditorScene;
		int                      windowWidth  = 1600;
		int                      windowHeight = 900;
		WindowMode               windowMode   = WindowMode::Windowed;
		bool                     vsync        = true;
		std::vector<std::string> scenes;

		static ProjectSettings& Get();
		static bool             IsProjectDirectory(const std::string& folder);
		static constexpr const char* FileName() { return "project.json"; }

		bool Open(const std::string& folder);
		bool Create(const std::string& folder, const std::string& projectName);
		bool Save() const;
		void Close();

		[[nodiscard]] bool               IsOpen() const { return m_open; }
		[[nodiscard]] const std::string& Root() const { return m_root; }
		[[nodiscard]] std::string        FilePath() const;
		[[nodiscard]] std::string        ScenesDirectory() const;
		[[nodiscard]] std::string        AssetsDirectory() const;
		[[nodiscard]] std::string        ScriptsDirectory() const;

		[[nodiscard]] int         SceneCount() const { return static_cast<int>(scenes.size()); }
		[[nodiscard]] std::string SourceScenePath(int index) const;
		[[nodiscard]] std::string RuntimeScenePath(int index) const;
		[[nodiscard]] int         FindSceneIndex(const std::string& path) const;

		void AddScene(const std::string& path);
		void RemoveScene(int index);
		void MoveScene(int index, int delta);

		std::string NormalizePath(const std::string& path) const;

		static const char* WindowModeName(WindowMode mode);
		static WindowMode  WindowModeFromName(const std::string& name);

	  private:
		bool        LoadFile();
		std::string m_root;
		bool        m_open = false;
	};

	inline ProjectSettings& GetProject() { return ProjectSettings::Get(); }

} // namespace Engine
