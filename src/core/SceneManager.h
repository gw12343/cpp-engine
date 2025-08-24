//
// Created by gabe on 8/16/25.
//

#pragma once

#include "core/module/Module.h"
#include "Scene.h"
#include "Entity.h"

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


		// Scene management
		std::unique_ptr<Scene> CreateScene(const std::string& name);

		// void RemoveScene(const std::string& name);

		//		std::shared_ptr<Scene> GetScene(const std::string& name);
		//
		void SetActiveScene(AssetHandle<Scene> scene);

		AssetHandle<Scene> GetActiveScene() { return m_activeScene; }

	  private:
		AssetHandle<Scene> m_activeScene;
	};

} // namespace Engine