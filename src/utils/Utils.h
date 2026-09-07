#pragma once

#include <spdlog/spdlog.h>

#include <cstdlib>
#include <string>

namespace Engine {
	void _GLCheckError(const char* file, int line);

	std::string GetFileName(std::string path);

#define ENGINE_GLCheckError() _GLCheckError(__FILE__, __LINE__)

#if defined(_MSC_VER)
#define ENGINE_DEBUG_BREAK() __debugbreak()
#elif defined(__GNUC__) || defined(__clang__)
#define ENGINE_DEBUG_BREAK() __builtin_trap()
#else
#define ENGINE_DEBUG_BREAK() std::raise(SIGTRAP)
#endif

#define ENGINE_ASSERT_IMPL(condition, message, ...)                                                                                                                                                                                            \
	do {                                                                                                                                                                                                                                       \
		if (!(condition)) {                                                                                                                                                                                                                    \
			spdlog::error("Assertion failed: ({})", #condition);                                                                                                                                                                               \
			spdlog::error("Message: " message, ##__VA_ARGS__);                                                                                                                                                                                 \
			spdlog::error("At {}:{} in function {}", __FILE__, __LINE__, __func__);                                                                                                                                                            \
			ENGINE_DEBUG_BREAK();                                                                                                                                                                                                              \
			std::abort();                                                                                                                                                                                                                      \
		}                                                                                                                                                                                                                                      \
	} while (false)

#define ENGINE_WARN_IMPL(message, ...)                                                                                                                                                                                                         \
	do {                                                                                                                                                                                                                                       \
		spdlog::warn("Message: " message, ##__VA_ARGS__);                                                                                                                                                                                      \
		spdlog::warn("At {}:{} in function {}", __FILE__, __LINE__, __func__);                                                                                                                                                                 \
	} while (false)

#ifndef NDEBUG
#define ENGINE_ASSERT(condition, ...) ENGINE_ASSERT_IMPL(condition, "" __VA_ARGS__)
#else
#define ENGINE_ASSERT(condition, ...) ((void) 0)
#endif

#ifndef NDEBUG
#define ENGINE_WARN(...) ENGINE_WARN_IMPL("" __VA_ARGS__)
#else
#define ENGINE_WARN(condition, ...) ((void) 0)
#endif

#define ENGINE_VERIFY(condition, ...) ENGINE_ASSERT_IMPL(condition, "" __VA_ARGS__)

} // namespace Engine
