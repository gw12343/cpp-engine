#pragma once

#include "Camera.h"
#include "Window.h"
#include "animation/AnimationManager.h"
#include "rendering/Renderer.h"
#include "sound/SoundManager.h"
#include "ui/UIManager.h"

#include <entt/entt.hpp>
#include <memory>
#include <spdlog/spdlog.h>

namespace Engine {

	// Forward declarations
	class Entity;
	class UIManager;

	class GEngine {
	  public:
		GEngine(int width, int height, const char* title);
		~GEngine();

		bool Initialize();
		void Run();
		void Shutdown();

		// Get the registry
		entt::registry&       GetRegistry() { return m_registry; }
		const entt::registry& GetRegistry() const { return m_registry; }

		// Get managers
		Audio::SoundManager& GetSoundManager() { return *m_soundManager; }
		AnimationManager&    GetAnimationManager() { return *m_animationManager; }

		// Get camera
		const Camera& GetCamera() const { return m_camera; }

		// Physics state
		bool IsPhysicsEnabled() const { return m_physicsEnabled; }

	  private:
		// Initialization methods
		bool InitializeWindow();
		bool InitializeRenderer();
		bool InitializeAudio();
		bool InitializePhysics();
		void CreateInitialEntities();

		// Update methods
		void ProcessInput();
		void Update();
		void UpdatePhysics();
		void RenderUI();

		Window                               m_window;
		Camera                               m_camera;
		std::unique_ptr<Renderer>            m_renderer;
		std::shared_ptr<spdlog::logger>      m_logger;
		std::shared_ptr<Audio::SoundManager> m_soundManager;
		std::unique_ptr<UIManager>           m_uiManager;
		std::unique_ptr<AnimationManager>    m_animationManager;

		// Audio-related members
		std::shared_ptr<Audio::SoundBuffer> m_backgroundMusic;
		std::shared_ptr<Audio::SoundSource> m_backgroundMusicSource;

		// EnTT registry for ECS
		entt::registry m_registry;

		float m_deltaTime;
		float m_lastFrame;
		bool  m_physicsEnabled = false; // Default physics to off

		// Friend declaration to allow Entity to access private members
		friend class Entity;
		friend class UIManager;
	};

} // namespace Engine
