//
// Created by gabe on 6/22/25.
//
#pragma once

#include <memory>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include "ImGuiLogSink.h"

namespace Engine {
	class Logger {
	  public:
		static std::shared_ptr<spdlog::logger> get(const std::string& name);
		static std::shared_ptr<ImGuiLogSink>   getImGuiSink();
	};
} // namespace Engine