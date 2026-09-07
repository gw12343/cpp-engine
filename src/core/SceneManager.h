//
// Created by gabe on 8/16/25.
//

#pragma once

#include "assets/AssetHandle.h"
#include "core/module/Module.h"
#include "Scene.h"
#include "Entity.h"

#include <string>

namespace Engine {
	class SceneManager : public Module {
	  public:
		SceneManager()           = default;
		~SceneManager() override = default;
		[[nodiscard]] std::string name() const override { return "SceneModule"; }


		// Module overrides
		void onInit() override;
		void onUpdate(float dt) override;
		void onGameStart() override {}
		void onShutdown() override;


		void UpdateTransforms();
		void UpdateTransformRecursive(Entity entity, const glm::mat4& parentMatrix, bool hasParent);
		void setLuaBindings() override;


		// Scene management
		std::unique_ptr<Scene> CreateScene(const std::string& name);

		void SetActiveScene(SceneHandle scene);
		void UnloadActive();
		bool LoadScenePath(const std::string& path, bool restartScripts);
		bool LoadSceneIndex(int index, bool restartScripts);

		// Queued until the end of the frame so scripts can finish Update.
		void RequestLoadScene(int index);
		void RequestLoadScenePath(const std::string& path);
		void FlushPendingSceneLoad();

		[[nodiscard]] int GetActiveBuildIndex() const { return m_activeBuildIndex; }

		SceneHandle GetActiveScene() { return m_activeScene; }

	  private:
		SceneHandle          m_activeScene;
		int                  m_activeBuildIndex = -1;
		bool                 m_hasPendingScene  = false;
		int                  m_pendingIndex     = -1;
		std::string          m_pendingPath;
	};

} // namespace Engine