//
// Created by gabe on 6/22/25.
//
#include "utils/Utils.h"
template <typename T>
void Engine::ModuleManager::RegisterModule()
{
	auto module = std::make_shared<T>();
	module->log = Logger::get(module->name());
	m_modules.push_back(module);
}

template <typename T>
void Engine::ModuleManager::RegisterExternal(std::shared_ptr<T> mod)
{
	ENGINE_ASSERT(mod, "module must not be null");
	static_assert(std::is_base_of<Engine::Module, T>::value, "T must inherit from Module");

	mod->log = Logger::get(mod->name());
	m_modules.push_back(std::static_pointer_cast<Engine::Module>(mod));
}