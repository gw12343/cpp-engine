#include "Shader.h"

#include <fstream>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <sstream>

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

	bool Shader::LoadFromFiles(const std::string& vertexPath, const std::string& fragmentPath)
	{
		// Read shader source code from files
		std::string vertexSource   = ReadFile(vertexPath);
		std::string fragmentSource = ReadFile(fragmentPath);

		if (vertexSource.empty() || fragmentSource.empty()) {
			spdlog::error("Failed to read shader files");
			return false;
		}

		// Create and compile shaders
		GLuint vertexShader   = 0;
		GLuint fragmentShader = 0;

		if (!CompileShader(vertexShader, GL_VERTEX_SHADER, vertexSource) || !CompileShader(fragmentShader, GL_FRAGMENT_SHADER, fragmentSource)) {
			return false;
		}

		// Create shader program
		programID = glCreateProgram();
		glAttachShader(programID, vertexShader);
		glAttachShader(programID, fragmentShader);

		// Link program
		if (!LinkProgram()) {
			glDeleteShader(vertexShader);
			glDeleteShader(fragmentShader);
			return false;
		}

		// Clean up shaders
		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);
		SPDLOG_INFO("Linked shader with id {}", programID);
		return true;
	}

	void Shader::Use() const
	{
		glUseProgram(programID);
	}

	void Shader::SetBool(const std::string& name, bool value) const
	{
		glUniform1i(glGetUniformLocation(programID, name.c_str()), (int) value);
	}

	void Shader::SetInt(const std::string& name, int value) const
	{
		glUniform1i(glGetUniformLocation(programID, name.c_str()), value);
	}

	void Shader::SetFloat(const std::string& name, float value) const
	{
		glUniform1f(glGetUniformLocation(programID, name.c_str()), value);
	}

	void Shader::SetVec3(const std::string& name, glm::vec3 value) const
	{
		glUniform3fv(glGetUniformLocation(programID, name.c_str()), 1, (GLfloat*) glm::value_ptr(value));
	}

	void Shader::SetMat4(const std::string& name, glm::mat4* value) const
	{
		glUniformMatrix4fv(glGetUniformLocation(programID, name.c_str()), 1, GL_FALSE, glm::value_ptr(*value));
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