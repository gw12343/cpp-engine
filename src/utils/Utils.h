#pragma once
#include "Effekseer/Effekseer.Matrix44.h"

#include "spdlog/spdlog.h"

#include <glm/glm.hpp>
#include <ozz/base/containers/vector.h>
#include <ozz/base/maths/simd_math.h>
#include <ozz/base/maths/soa_transform.h>
#include <filesystem>
#include <string>


namespace Engine {
	ozz::math::Float4x4   FromMatrix(const glm::mat4& glmMatrix);
	::Effekseer::Matrix44 ConvertGLMToEffekseerMatrix(const glm::mat4& glmMatrix);

	void _GLCheckError(const char* file, int line);

	std::string GetFileName(std::string path);
#define ENGINE_GLCheckError() _GLCheckError(__FILE__, __LINE__)

	// Platform-specific debug break
#if defined(_MSC_VER)
#define ENGINE_DEBUG_BREAK() __debugbreak()
#elif defined(__GNUC__) || defined(__clang__)
#define ENGINE_DEBUG_BREAK() __builtin_trap()
#else
#define ENGINE_DEBUG_BREAK() std::raise(SIGTRAP)
#endif

// Internal macro to format and log the failure
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


// ENGINE_ASSERT only in debug mode
#ifndef NDEBUG
#define ENGINE_ASSERT(condition, ...) ENGINE_ASSERT_IMPL(condition, "" __VA_ARGS__)
#else
#define ENGINE_ASSERT(condition, ...) ((void) 0)
#endif

// ENGINE_ASSERT only in debug mode
#ifndef NDEBUG
#define ENGINE_WARN(...) ENGINE_WARN_IMPL("" __VA_ARGS__)
#else
#define ENGINE_WARN(condition, ...) ((void) 0)
#endif

// ENGINE_VERIFY always runs (can be used for runtime checks)
#define ENGINE_VERIFY(condition, ...) ENGINE_ASSERT_IMPL(condition, "" __VA_ARGS__)


} // namespace Engine