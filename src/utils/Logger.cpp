//
// Created by gabe on 6/22/25.
//

#include "Logger.h"
#include <spdlog/sinks/basic_file_sink.h>
#include <unordered_map>

namespace Engine {

	std::unordered_map<std::string, std::shared_ptr<spdlog::logger>> loggers;

	std::shared_ptr<spdlog::logger> Logger::get(const std::string& name)
	{
		if (loggers.find(name) == loggers.end()) {
			auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
			sink->set_pattern("[%T] [%^%n%$] %v");
			loggers[name] = std::make_shared<spdlog::logger>(name, sink);
			loggers[name]->set_level(spdlog::level::debug);
			spdlog::register_logger(loggers[name]);
		}
		return loggers[name];
	}
} // namespace Engine