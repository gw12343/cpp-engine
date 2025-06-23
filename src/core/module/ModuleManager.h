//
// Created by gabe on 6/22/25.
//

#pragma once
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>
#include "Module.h"

namespace Engine {
	class ModuleManager {
	  public:
		template <typename T>
		void RegisterModule();

		template <typename T>
		void RegisterExternal(std::shared_ptr<T> mod);

		void InitAll();
		void InitAllLuaBindings();
		void UpdateAll(float dt);
		void ShutdownAll();

	  private:
		std::vector<std::shared_ptr<Module>> m_modules;
	};
} // namespace Engine
#include "ModuleManager.inl"