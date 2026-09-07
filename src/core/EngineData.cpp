//
// Created by gabe on 6/22/25.
//

#include "EngineData.h"

#include "SceneManager.h"
#include "ThreadPool.h"

namespace Engine {
	EngineData::EngineData()  = default;
	EngineData::~EngineData() = default;

	EngineData& Get()
	{
		static EngineData instance;
		return instance;
	}

	ThreadPool& GetThreadPool()
	{
		return *Get().threadPool;
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