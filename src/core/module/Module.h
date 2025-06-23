#pragma once
#include <string>
#include <memory>
#include "utils/Logger.h"

namespace Engine {
	class Module {
	  public:
		virtual ~Module()                                    = default;
		virtual void                      onInit()           = 0;
		virtual void                      onUpdate(float dt) = 0;
		virtual void                      onShutdown()       = 0;
		virtual void                      setLuaBindings() {}
		[[nodiscard]] virtual std::string name() const = 0;
		std::shared_ptr<spdlog::logger>   log;
	};
} // namespace Engine