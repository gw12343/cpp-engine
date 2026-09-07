//
// Created by gabe on 6/22/25.
//
#pragma once

#include <spdlog/spdlog.h>

#include <memory>
#include <string>

class ImGuiLogSink;

namespace Engine {
	class Logger {
	  public:
		static std::shared_ptr<spdlog::logger> get(const std::string& name);
		static std::shared_ptr<ImGuiLogSink>   getImGuiSink();
		static void                            Shutdown();
	};
} // namespace Engine
