#pragma once

#include <entt/entt.hpp>
#include <memory>
#include <spdlog/spdlog.h>

namespace Engine {

	// Forward declaration
	class Entity;

	/**
	 * @brief Core engine class managing the game loop, systems, and global state.
	 */
	class GEngine {
	  public:
		/**
		 * @brief Constructs the engine and initializes the window.
		 * @param width The width of the window in pixels.
		 * @param height The height of the window in pixels.
		 * @param title The window title.
		 */
		GEngine(int width, int height, const char* title);

		/**
		 * @brief Destroys the engine instance.
		 */
		~GEngine() = default;

		/**
		 * @brief Initializes subsystems and resources.
		 * @return True if initialization was successful, false otherwise.
		 */
		bool Initialize();

		/**
		 * @brief Starts and runs the main engine loop.
		 */
		void Run();

		/**
		 * @brief Shuts down the engine and releases all resources.
		 */
		void Shutdown();

		void Tick();

		/** Static accessor for the engine instance */
		static GEngine& GetInstance() { return *s_instance; }

	  private:
		static GEngine* s_instance; // <--- Add this
		/**
		 * @brief Creates initial entities used in the game world.
		 */
		static void LoadGameAssets();

		std::shared_ptr<spdlog::logger> m_logger; ///< Logger instance.


		float m_deltaTime; ///< Time elapsed since last frame.
		float m_lastFrame; ///< Timestamp of last frame.
	};

} // namespace Engine
