//
// Created by gabe on 6/22/25.
//


#include "ModuleManager.h"
#include "core/EngineData.h"

namespace Engine {


	void ModuleManager::InitAll()
	{
		for (auto& module : m_modules) {
			module->log->debug("Initializing...");
			module->onInit();
		}
	}

	void ModuleManager::StartGame()
	{
		GetDefaultLogger()->info("Starting Game");
		for (auto& module : m_modules) {
			module->onGameStart();
		}
	}

	void ModuleManager::InitAllLuaBindings()
	{
		for (auto& module : m_modules) {
			module->setLuaBindings();
		}
	}

	void ModuleManager::UpdateAll(float dt)
	{
		for (auto& module : m_modules) {
			module->onUpdate(dt);
		}
	}

	void ModuleManager::ShutdownAll()
	{
		for (auto it = m_modules.rbegin(); it != m_modules.rend(); ++it) {
			(*it)->log->debug("Shutting down...");
			(*it)->onShutdown();
		}
	}

	void ModuleManager::Clear()
	{
		m_modules.clear();
	}
} // namespace Engine