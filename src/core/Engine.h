#pragma once

#include "Camera.h"
#include "Window.h"
#include "animation/AnimationManager.h"
#include "rendering/Renderer.h"
#include "rendering/particles/ParticleManager.h"
#include "sound/SoundManager.h"
#include "terrain/TerrainManager.h"
#include "ui/UIManager.h"

#include <entt/entt.hpp>
#include <memory>
#include <spdlog/spdlog.h>

namespace Engine {

	// Forward declarations
	class Entity;

	class GEngine {
	  public:
		GEngine(int width, int height, const char* title);
		~GEngine() = default;

		bool Initialize();
		void Run();
		void Shutdown();

		// Get the registry
		entt::registry&                                      GetRegistry() { return m_registry; }
		[[maybe_unused]] [[nodiscard]] const entt::registry& GetRegistry() const { return m_registry; }

		// Get managers
		Audio::SoundManager& GetSoundManager() { return *m_soundManager; }
		AnimationManager&    GetAnimationManager() { return *m_animationManager; }
		ParticleManager&     GetParticleManager() { return *m_particleManager; }

		// Get camera
		[[nodiscard]] const Camera& GetCamera() const { return m_camera; }

		// Physics state
		[[nodiscard]] bool IsPhysicsEnabled() const { return m_physicsEnabled; }

		static Terrain::TerrainManager terrainManager;

	  private:
		bool InitializeRenderer();
		void CreateInitialEntities();

		// Update methods
		void ProcessInput();
		void Update();

		Window                               m_window;
		Camera                               m_camera;
		std::shared_ptr<spdlog::logger>      m_logger;
		std::shared_ptr<Audio::SoundManager> m_soundManager;
		std::unique_ptr<Renderer>            m_renderer;
		std::unique_ptr<UI::UIManager>       m_uiManager;
		std::unique_ptr<AnimationManager>    m_animationManager;
		std::unique_ptr<ParticleManager>     m_particleManager;


		// EnTT registry for ECS
		entt::registry m_registry;

		float m_deltaTime;
		float m_lastFrame;
		bool  m_physicsEnabled = false; // Default physics to off

		// Friend declaration to allow Entity to access private members
		friend class Entity;
	};

} // namespace Engine
