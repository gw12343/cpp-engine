#pragma once

#include <string>

namespace Engine::UI {

#ifdef GAME_BUILD
	inline void DrawProjectMenu() {}
	inline void DrawProjectWindows() {}
	inline bool OpenProjectFolder(const std::string&) { return false; }
	inline bool CreateProjectFolder(const std::string&, const std::string&) { return false; }
#else
	void DrawProjectMenu();
	void DrawProjectWindows();
	bool OpenProjectFolder(const std::string& folder);
	bool CreateProjectFolder(const std::string& folder, const std::string& name);
#endif

} // namespace Engine::UI
