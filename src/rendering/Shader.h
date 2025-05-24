#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <spdlog/spdlog.h>
#include <string>

namespace Engine {
	class Shader {
	  public:
		Shader();
		~Shader();

		// Load and compile shaders from source files
		bool LoadFromFiles(const std::string& vertexPath, const std::string& fragmentPath);
		// Load and compile shaders from in-memory source strings
		bool LoadFromSource(const std::string& vertexSource, const std::string& fragmentSource);
		
		// Use the shader program
		void Use() const;

		// Utility uniform functions
		void SetBool(const std::string& name, bool value) const;
		void SetInt(const std::string& name, int value) const;
		void SetFloat(const std::string& name, float value) const;
		void SetVec3(const std::string& name, glm::vec3 value) const;
		void SetMat4(const std::string& name, glm::mat4* value) const;

		// Get the program ID
		[[maybe_unused]] [[nodiscard]] GLuint GetProgramID() const { return programID; }

	  private:
		GLuint programID;

		// Helper functions
		bool        CompileShader(GLuint& shader, GLenum type, const std::string& source);
		bool        LinkProgram();
		void        CheckShaderError(GLuint shader, GLenum whatToCheck, const std::string& errorMessage);
		void        CheckProgramError(GLuint program, GLenum whatToCheck, const std::string& errorMessage);
		std::string ReadFile(const std::string& filePath);
	};
} // namespace Engine