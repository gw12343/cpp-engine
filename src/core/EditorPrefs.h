#pragma once

#include <string>
#include <vector>

namespace Engine {

	class EditorPrefs {
	  public:
		std::string              lastProject;
		std::vector<std::string> recentProjects;
		int                      editorWidth  = 1600;
		int                      editorHeight = 1200;

		static EditorPrefs& Get();
		static std::string  FilePath();

		bool Load();
		bool Save() const;
		void RememberProject(const std::string& folder);
	};

	inline EditorPrefs& GetEditorPrefs() { return EditorPrefs::Get(); }

} // namespace Engine
