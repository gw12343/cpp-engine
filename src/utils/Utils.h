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
} // namespace Engine