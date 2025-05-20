#pragma once

#include "Shader.h"
#include "Texture.h"

#include <glad/glad.h>
#include <memory>
#include <string>

namespace Engine {
	class Skybox {
	  public:
		Skybox();
		~Skybox();

		bool LoadFromFile(const std::string& path);
		void Draw(const Shader& shader) const;

	  private:
		void SetupMesh();

		GLuint                   m_vao;
		GLuint                   m_vbo;
		std::unique_ptr<Texture> m_texture;
		bool                     m_initialized;
	};
} // namespace Engine