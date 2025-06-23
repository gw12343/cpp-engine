//
// Created by gabe on 6/22/25.
//

#pragma once
#include <memory>
#include "entt/entt.hpp"
#include "Input.h"

namespace Engine {
	class Window;
	class Renderer;
	namespace Audio {
		class SoundManager;
	}
	class AnimationManager;
	class ParticleManager;
	namespace Terrain {
		class TerrainManager;
	}
	namespace UI {
		class UIManager;
	}
	class ScriptManager;
	class PhysicsManager;
	class Camera;

	class EngineData {
	  public:
		std::shared_ptr<Window>                  window;
		std::shared_ptr<Renderer>                renderer;
		std::shared_ptr<Audio::SoundManager>     sound;
		std::shared_ptr<AnimationManager>        animation;
		std::shared_ptr<ParticleManager>         particle;
		std::shared_ptr<Terrain::TerrainManager> terrain;
		std::shared_ptr<ScriptManager>           script;
		std::shared_ptr<UI::UIManager>           ui;
		std::shared_ptr<PhysicsManager>          physics;
		std::shared_ptr<Camera>                  camera;
		std::shared_ptr<Input>                   input;
		std::shared_ptr<entt::registry>          registry;
		bool                                     isPhysicsPaused = true;
	};

	EngineData& Get();

	inline bool IsPhysicsPaused()
	{
		return Get().isPhysicsPaused;
	}

	// Convenience inline accessors
	inline auto& GetWindow()
	{
		return *Get().window;
	}
	inline auto& GetRenderer()
	{
		return *Get().renderer;
	}
	inline auto& GetSoundManager()
	{
		return *Get().sound;
	}
	inline auto& GetAnimationManager()
	{
		return *Get().animation;
	}
	inline auto& GetParticleManager()
	{
		return *Get().particle;
	}
	inline auto& GetTerrainManager()
	{
		return *Get().terrain;
	}
	inline auto& GetScriptManager()
	{
		return *Get().script;
	}
	inline auto& GetPhysics()
	{
		return *Get().physics;
	}
	inline auto& GetCamera()
	{
		return *Get().camera;
	}
	inline auto& GetInput()
	{
		return *Get().input;
	}
	inline auto& GetRegistry()
	{
		return *Get().registry;
	}


} // namespace Engine
