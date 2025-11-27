//
// Created by gabe on 6/22/25.
//

#pragma once
#include <memory>
#include "entt/entt.hpp"
#include "assets/AssetHandle.h"
#include "utils/Logger.h"

#ifndef GAME_BUILD
#define SCENE_LOADER JSONSceneLoader
#define SCENE1 "scenes/scene1.json"
#else
#define SCENE_LOADER BinarySceneLoader
#define SCENE1 "scenes/scene1.bin"
#endif

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
	class PhysicsManager;
	class Camera;
	class AssetManager;
	class Scene;
	class Input;

	class ModuleManager;

	enum EngineState { EDITOR, PAUSED, PLAYING };

	class EngineData {
	  public:
		std::shared_ptr<AssetManager>            assetManager;
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
		EngineState                              state;
		ModuleManager*                           manager;

#ifndef GAME_BUILD
		std::unique_ptr<class EditorCommandStack> editorCommandStack;
		std::unique_ptr<class EntityClipboard>    entityClipboard;
#endif
	};

	EngineData& Get();


	inline auto GetDefaultLogger()
	{
		return Logger::get("core");
	}

	inline EngineState GetState()
	{
		return Get().state;
	}

	inline void SetState(EngineState st)
	{
		Get().state = st;
	}

	// Convenience inline accessors
	inline auto& GetAssetManager()
	{
		return *Get().assetManager;
	}

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
	inline auto& GetUI()
	{
		return *Get().ui;
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

#ifndef GAME_BUILD
	inline auto& GetCommandStack()
	{
		return *Get().editorCommandStack;
	}

	inline auto& GetClipboard()
	{
		return *Get().entityClipboard;
	}
#endif

	entt::registry& GetCurrentSceneRegistry();
	Scene*          GetCurrentScene();
} // namespace Engine
