//
// Created by gabe on 6/22/25.
//

#include "ModuleManager.h"

namespace Engine {
	void ModuleManager::InitAll()
	{
		for (auto& module : m_modules) {
			module->log->info("Initializing...");
			module->onInit();
		}
	}

	void ModuleManager::InitAllLuaBindings()
	{
		for (auto& module : m_modules) {
			module->log->info("Initializing Lua Bindings...");
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
		for (auto& module : m_modules) {
			module->log->info("Shutting down...");
			module->onShutdown();
		}
	}
} // namespace Engine