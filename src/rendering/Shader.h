#pragma once


#include <glm/glm.hpp>
#include <spdlog/spdlog.h>
#include <string>
#include <optional>

typedef unsigned int GLuint;
typedef unsigned int GLenum;


namespace Engine {
	class Shader {
	  public:
		Shader();
		~Shader();

		// Load and compile shaders from source files
		bool LoadFromFiles(const std::string& vertexPath, const std::string& fragmentPath, const std::optional<std::string>& geometryPath);
		// Load and compile shaders from in-memory source strings
		bool LoadFromSource(const std::string& vertexSource, const std::string& fragmentSource);

		// Bind the shader program
		void Bind() const;

		// Utility uniform functions
		void SetBool(const std::string& name, bool value) const;
		void SetInt(const std::string& name, int value) const;
		void SetFloat(const std::string& name, float value) const;
		void SetVec2(const std::string& name, glm::vec2 value) const;
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