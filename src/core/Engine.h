#pragma once

#include <memory>

namespace Engine {
	class HotReloadWatcher;
	class ModuleManager;
	class Entity;
}

namespace efsw {
	class FileWatcher;
}

namespace spdlog {
	class logger;
}

namespace Engine {

	class GEngine {
	  public:
		GEngine(int width, int height, const char* title);

		~GEngine();

		bool Initialize();

		void Run();

		void Shutdown();

	  private:
		void LoadGameAssets();

		std::shared_ptr<spdlog::logger> m_logger;

		float m_deltaTime;
		float m_lastFrame;

		std::unique_ptr<ModuleManager> m_moduleManager;

		std::unique_ptr<efsw::FileWatcher> m_assetFileWatcher;
		std::unique_ptr<HotReloadWatcher>  m_assetWatcher;
	};

} // namespace Engine
