#include "core/Engine.h"
#include "core/EditorPrefs.h"
#include "core/EnginePaths.h"
#include "core/ProjectSettings.h"

#include <filesystem>


using namespace Engine;

int main()
{
	spdlog::info("Running in {}", std::filesystem::current_path().string());
	GetEnginePaths().Detect();
	spdlog::info("Engine root {}", GetEnginePaths().EngineRoot());

#ifdef GAME_BUILD
	GetProject().Open(std::filesystem::current_path().string());
	const auto& project = GetProject();
	GEngine     engine(project.windowWidth, project.windowHeight, project.name.c_str());
#else
	auto& prefs = GetEditorPrefs();
	prefs.Load();
	if (ProjectSettings::IsProjectDirectory(prefs.lastProject)) {
		GetProject().Open(prefs.lastProject);
	}

	GEngine engine(prefs.editorWidth, prefs.editorHeight, "cpp-engine");
#endif

	if (!engine.Initialize()) {
		spdlog::critical("Failed to init engine");
		return -1;
	}

	engine.Run();
	engine.Shutdown();
	return 0;
}
