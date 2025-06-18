#pragma once

#include "Camera.h"
#include "Window.h"
#include "animation/AnimationManager.h"
#include "rendering/Renderer.h"
#include "rendering/particles/ParticleManager.h"
#include "sound/SoundManager.h"
#include "terrain/TerrainManager.h"
#include "rendering/ui/UIManager.h"

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

		/**
		 * @brief Access the EnTT registry.
		 * @return A reference to the entity registry.
		 */
		entt::registry& GetRegistry() { return m_registry; }

		/**
		 * @brief Const access to the EnTT registry.
		 * @return A const reference to the entity registry.
		 */
		[[maybe_unused]] [[nodiscard]] const entt::registry& GetRegistry() const { return m_registry; }

		/**
		 * @brief Retrieves the global sound manager.
		 * @return Reference to the SoundManager.
		 */
		Audio::SoundManager& GetSoundManager() { return *m_soundManager; }

		/**
		 * @brief Retrieves the global animation manager.
		 * @return Reference to the AnimationManager.
		 */
		AnimationManager& GetAnimationManager() { return *m_animationManager; }

		/**
		 * @brief Retrieves the global particle manager.
		 * @return Reference to the ParticleManager.
		 */
		ParticleManager& GetParticleManager() { return *m_particleManager; }

		/**
		 * @brief Retrieves the current camera.
		 * @return Const reference to the Camera object.
		 */
		[[nodiscard]] Camera& GetCamera() { return m_camera; }

		/**
		 * @brief Checks if physics is enabled.
		 * @return True if physics is enabled, false otherwise.
		 */
		[[nodiscard]] bool IsPhysicsEnabled() const { return m_physicsEnabled; }

		/**
		 * @brief Static terrain manager instance.
		 */
		static Terrain::TerrainManager terrainManager;

		/**
		 * @brief Window used for rendering.
		 */
		Window m_window;

	  private:
		/**
		 * @brief Initializes the rendering system.
		 * @return True if successful, false otherwise.
		 */
		bool InitializeRenderer();

		/**
		 * @brief Creates initial entities used in the game world.
		 */
		void CreateInitialEntities();

		/**
		 * @brief Handles input events (keyboard, mouse, etc.).
		 */
		void ProcessInput();

		/**
		 * @brief Updates all engine systems and entities.
		 */
		void Update();

		Camera                               m_camera;           ///< Main game camera.
		std::shared_ptr<spdlog::logger>      m_logger;           ///< Logger instance.
		std::shared_ptr<Audio::SoundManager> m_soundManager;     ///< Manages all audio.
		std::unique_ptr<Renderer>            m_renderer;         ///< Renders scenes.
		std::unique_ptr<UI::UIManager>       m_uiManager;        ///< Manages UI rendering.
		std::unique_ptr<AnimationManager>    m_animationManager; ///< Controls animations.
		std::unique_ptr<ParticleManager>     m_particleManager;  ///< Manages particle effects.

		entt::registry m_registry; ///< Entity-component registry (ECS).

		float m_deltaTime;              ///< Time elapsed since last frame.
		float m_lastFrame;              ///< Timestamp of last frame.
		bool  m_physicsEnabled = false; ///< Whether physics simulation is active.

		/**
		 * @brief Grants Entity class access to private members.
		 */
		friend class Entity;
	};

} // namespace Engine
