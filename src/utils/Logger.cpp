//
// Created by gabe on 6/22/25.
//

#include "Logger.h"
#include <spdlog/sinks/basic_file_sink.h>
#include <unordered_map>

namespace Engine {

	std::unordered_map<std::string, std::shared_ptr<spdlog::logger>> loggers;
	static std::shared_ptr<ImGuiLogSink>                             imguiSink = std::make_shared<ImGuiLogSink>();


	std::shared_ptr<spdlog::logger> Logger::get(const std::string& name)
	{
		auto it = loggers.find(name);
		if (it != loggers.end()) return it->second;

		// Console sink (stdout)
		auto stdoutSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
		stdoutSink->set_pattern("[%T] [%^%n%$] %v");

		// Combine sinks (stdout + ImGui)
		std::vector<spdlog::sink_ptr> sinks;
		sinks.push_back(stdoutSink);
		sinks.push_back(imguiSink);

		// Create logger with both sinks
		auto logger = std::make_shared<spdlog::logger>(name, sinks.begin(), sinks.end());
		logger->set_level(spdlog::level::debug);

		spdlog::register_logger(logger);
		loggers[name] = logger;

		return logger;
	}

	// Provide access to the ImGui sink so UI can draw it
	std::shared_ptr<ImGuiLogSink> Logger::getImGuiSink()
	{
		return imguiSink;
	}
} // namespace Engine