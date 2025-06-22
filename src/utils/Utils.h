#pragma once
#include "Effekseer/Effekseer.Matrix44.h"
#include "glad/glad.h"
#include "spdlog/spdlog.h"

#include <glm/glm.hpp>
#include <ozz/base/containers/vector.h>
#include <ozz/base/maths/simd_math.h>
#include <ozz/base/maths/soa_transform.h>

namespace Engine {
	ozz::math::Float4x4   FromMatrix(const glm::mat4& glmMatrix);
	::Effekseer::Matrix44 ConvertGLMToEffekseerMatrix(const glm::mat4& glmMatrix);
	inline void           _GLCheckError(const char* file, int line)
	{
		GLenum err;
		while ((err = glGetError()) != GL_NO_ERROR) {
			const char* errorStr = "UNKNOWN_ERROR";
			switch (err) {
				case GL_INVALID_ENUM:
					errorStr = "GL_INVALID_ENUM";
					break;
				case GL_INVALID_VALUE:
					errorStr = "GL_INVALID_VALUE";
					break;
				case GL_INVALID_OPERATION:
					errorStr = "GL_INVALID_OPERATION";
					break;
				case GL_STACK_OVERFLOW:
					errorStr = "GL_STACK_OVERFLOW";
					break;
				case GL_STACK_UNDERFLOW:
					errorStr = "GL_STACK_UNDERFLOW";
					break;
				case GL_OUT_OF_MEMORY:
					errorStr = "GL_OUT_OF_MEMORY";
					break;
				case GL_INVALID_FRAMEBUFFER_OPERATION:
					errorStr = "GL_INVALID_FRAMEBUFFER_OPERATION";
					break;
			}

			spdlog::error("[OpenGL Error] {} (0x{:X}) in {} at line {}", errorStr, err, file, line);
		}
	}

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

// ENGINE_ASSERT only in debug mode
#ifndef NDEBUG
#define ENGINE_ASSERT(condition, ...) ENGINE_ASSERT_IMPL(condition, "" __VA_ARGS__)
#else
#define ENGINE_ASSERT(condition, ...) ((void) 0)
#endif

// ENGINE_VERIFY always runs (can be used for runtime checks)
#define ENGINE_VERIFY(condition, ...) ENGINE_ASSERT_IMPL(condition, "" __VA_ARGS__)


} // namespace Engine