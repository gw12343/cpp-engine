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
	class SceneManager;
	// class SerializationManager;
	class PhysicsManager;
	class Camera;
	class AssetManager;
	class Scene;

	class EngineData {
	  public:
		std::shared_ptr<AssetManager> assetManager;
		//	std::shared_ptr<SerializationManager>    serialization;
		std::shared_ptr<SceneManager>            scene;
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
		// std::shared_ptr<entt::registry>          registry;
	};

	EngineData& Get();

	// Convenience inline accessors
	inline auto& GetAssetManager()
	{
		return *Get().assetManager;
	}

	//	inline auto& GetSerializationManager()
	//	{
	//		return *Get().serialization;
	//	}

	inline auto& GetSceneManager()
	{
		return *Get().scene;
	}

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

	entt::registry& GetCurrentSceneRegistry();
	Scene*          GetCurrentScene();

	//	inline auto& GetRegistry()
	//	{
	//		return *Get().registry;
	//	}


} // namespace Engine
