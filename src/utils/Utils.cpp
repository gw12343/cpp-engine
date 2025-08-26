#include "utils/Utils.h"
#include "glad/glad.h"

namespace Engine {
	ozz::math::Float4x4 FromMatrix(const glm::mat4& glmMatrix)
	{
		return ozz::math::Float4x4{ozz::math::simd_float4::Load(glmMatrix[0][0], glmMatrix[0][1], glmMatrix[0][2], glmMatrix[0][3]),
		                           ozz::math::simd_float4::Load(glmMatrix[1][0], glmMatrix[1][1], glmMatrix[1][2], glmMatrix[1][3]),
		                           ozz::math::simd_float4::Load(glmMatrix[2][0], glmMatrix[2][1], glmMatrix[2][2], glmMatrix[2][3]),
		                           ozz::math::simd_float4::Load(glmMatrix[3][0], glmMatrix[3][1], glmMatrix[3][2], glmMatrix[3][3])};
	}

	::Effekseer::Matrix44 ConvertGLMToEffekseerMatrix(const glm::mat4& glmMatrix)
	{
		::Effekseer::Matrix44 effekseerMatrix;
		for (int column = 0; column < 4; ++column) {
			for (int row = 0; row < 4; ++row) {
				effekseerMatrix.Values[column][row] = glmMatrix[column][row];
			}
		}
		return effekseerMatrix;
	}

	std::string GetFileName(std::string path)
	{
		return std::filesystem::path(path).filename().string();
	}

	void _GLCheckError(const char* file, int line)
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
			spdlog::log(spdlog::source_loc{file, line, SPDLOG_FUNCTION}, spdlog::level::err, "[OpenGL Error] {} (0x{:X})", errorStr, err);
		}
		return;
	}
} // namespace Engine