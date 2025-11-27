//
// Created by gabe on 6/22/25.
//

#include "EngineData.h"
#include "Scene.h"
#include "Engine.h"

#ifndef GAME_BUILD
#include "EditorCommandStack.h"
#include "EntityClipboard.h"
#endif
#include "assets/AssetManager.h"
#include "SceneManager.h"

namespace Engine {
	EngineData& Get()
	{
		static EngineData instance;
		return instance;
	}

	entt::registry& GetCurrentSceneRegistry()
	{
		auto* scene = GetCurrentScene();
		return *scene->GetRegistry();
	}
	Scene* GetCurrentScene()
	{
		return GetAssetManager().Get(GetSceneManager().GetActiveScene());
	}

} // namespace Engine

#include "assets/AssetManager.inl"