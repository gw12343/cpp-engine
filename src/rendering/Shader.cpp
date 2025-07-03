#include "Shader.h"
#include "utils/Utils.h"

#include <fstream>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <sstream>
#include <glad/glad.h>

namespace Engine {
	Shader::Shader() : programID(0)
	{
	}

	Shader::~Shader()
	{
		if (programID != 0) {
			glDeleteProgram(programID);
		}
	}

	bool Shader::LoadFromFiles(const std::string& vertexPath, const std::string& fragmentPath, const std::optional<std::string>& geometryPath)
	{
		// Read shader source code from files
		std::string vertexSource   = ReadFile(vertexPath);
		std::string fragmentSource = ReadFile(fragmentPath);
		std::string geometrySource;

		if (vertexSource.empty() || fragmentSource.empty()) {
			spdlog::error("Failed to read vertex or fragment shader files");
			return false;
		}

		if (geometryPath.has_value()) {
			geometrySource = ReadFile(*geometryPath);
			if (geometrySource.empty()) {
				spdlog::error("Failed to read geometry shader file");
				return false;
			}
		}

		// Create and compile shaders
		GLuint vertexShader   = 0;
		GLuint fragmentShader = 0;
		GLuint geometryShader = 0;

		if (!CompileShader(vertexShader, GL_VERTEX_SHADER, vertexSource) || !CompileShader(fragmentShader, GL_FRAGMENT_SHADER, fragmentSource)) {
			return false;
		}

		if (geometryPath.has_value()) {
			if (!CompileShader(geometryShader, GL_GEOMETRY_SHADER, geometrySource)) {
				glDeleteShader(vertexShader);
				glDeleteShader(fragmentShader);
				return false;
			}
		}

		// Create shader program
		programID = glCreateProgram();
		glAttachShader(programID, vertexShader);
		glAttachShader(programID, fragmentShader);
		if (geometryPath.has_value()) {
			glAttachShader(programID, geometryShader);
		}

		// Link program
		if (!LinkProgram()) {
			glDeleteShader(vertexShader);
			glDeleteShader(fragmentShader);
			if (geometryPath.has_value()) glDeleteShader(geometryShader);
			return false;
		}

		// Clean up shaders
		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);
		if (geometryPath.has_value()) glDeleteShader(geometryShader);

		SPDLOG_INFO("Linked shader with id {}", programID);
		ENGINE_GLCheckError();
		return true;
	}

	bool Shader::LoadFromSource(const std::string& vertexSource, const std::string& fragmentSource)
	{
		SPDLOG_INFO("LOADING TERRAIN SHADER FROM SOURCE");
		if (vertexSource.empty() || fragmentSource.empty()) {
			spdlog::error("Shader source code is empty");
			return false;
		}

		GLuint vertexShader   = 0;
		GLuint fragmentShader = 0;

		if (!CompileShader(vertexShader, GL_VERTEX_SHADER, vertexSource) || !CompileShader(fragmentShader, GL_FRAGMENT_SHADER, fragmentSource)) {
			return false;
		}

		programID = glCreateProgram();
		glAttachShader(programID, vertexShader);
		glAttachShader(programID, fragmentShader);

		if (!LinkProgram()) {
			glDeleteShader(vertexShader);
			glDeleteShader(fragmentShader);
			return false;
		}

		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);
		SPDLOG_INFO("Linked shader from source with id {}", programID);
		ENGINE_GLCheckError();
		return true;
	}


	void Shader::Bind() const
	{
		glUseProgram(programID);
		ENGINE_GLCheckError();
	}

	void Shader::SetBool(const std::string& name, bool value) const
	{
		glUniform1i(glGetUniformLocation(programID, name.c_str()), (int) value);
		ENGINE_GLCheckError();
	}

	void Shader::SetInt(const std::string& name, int value) const
	{
		glUniform1i(glGetUniformLocation(programID, name.c_str()), value);
		ENGINE_GLCheckError();
	}

	void Shader::SetFloat(const std::string& name, float value) const
	{
		glUniform1f(glGetUniformLocation(programID, name.c_str()), value);
		ENGINE_GLCheckError();
	}

	void Shader::SetVec3(const std::string& name, glm::vec3 value) const
	{
		glUniform3fv(glGetUniformLocation(programID, name.c_str()), 1, (GLfloat*) glm::value_ptr(value));
		ENGINE_GLCheckError();
	}

	void Shader::SetVec2(const std::string& name, glm::vec2 value) const
	{
		glUniform2fv(glGetUniformLocation(programID, name.c_str()), 1, (GLfloat*) glm::value_ptr(value));
		ENGINE_GLCheckError();
	}

	void Shader::SetMat4(const std::string& name, glm::mat4* value) const
	{
		glUniformMatrix4fv(glGetUniformLocation(programID, name.c_str()), 1, GL_FALSE, glm::value_ptr(*value));
		ENGINE_GLCheckError();
	}

	bool Shader::CompileShader(GLuint& shader, GLenum type, const std::string& source)
	{
		shader                 = glCreateShader(type);
		const char* sourceCStr = source.c_str();
		glShaderSource(shader, 1, &sourceCStr, nullptr);
		glCompileShader(shader);

		CheckShaderError(shader, GL_COMPILE_STATUS, "Shader compilation failed");

		GLint success;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		return success == GL_TRUE;
	}

	bool Shader::LinkProgram()
	{
		glLinkProgram(programID);
		CheckProgramError(programID, GL_LINK_STATUS, "Program linking failed");

		GLint success;
		glGetProgramiv(programID, GL_LINK_STATUS, &success);
		return success == GL_TRUE;
	}

	void Shader::CheckShaderError(GLuint shader, GLenum whatToCheck, const std::string& errorMessage)
	{
		GLint success;
		glGetShaderiv(shader, whatToCheck, &success);

		if (success == GL_FALSE) {
			GLint logLength;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);

			std::vector<GLchar> errorLog(logLength);
			glGetShaderInfoLog(shader, logLength, nullptr, &errorLog[0]);

			spdlog::error("{}: {}", errorMessage, &errorLog[0]);
		}
	}

	void Shader::CheckProgramError(GLuint program, GLenum whatToCheck, const std::string& errorMessage)
	{
		GLint success;
		glGetProgramiv(program, whatToCheck, &success);

		if (success == GL_FALSE) {
			GLint logLength;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);

			std::vector<GLchar> errorLog(logLength);
			glGetProgramInfoLog(program, logLength, nullptr, &errorLog[0]);

			spdlog::error("{}: {}", errorMessage, &errorLog[0]);
		}
	}

	std::string Shader::ReadFile(const std::string& filePath)
	{
		std::ifstream file(filePath);
		if (!file.is_open()) {
			spdlog::error("Failed to open file: {}", filePath);
			return "";
		}

		std::stringstream buffer;
		buffer << file.rdbuf();
		return buffer.str();
	}
} // namespace Engine