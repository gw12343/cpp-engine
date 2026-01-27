#pragma once

#include "Shader.h"
#include "Texture.h"


#include <memory>
#include <string>

typedef unsigned int GLuint;

namespace Engine {
	class Skybox {
	  public:
		Skybox();
		~Skybox();

		bool LoadFromFile(const std::string& path);
		void Draw(const Shader& shader) const;

		std::unique_ptr<Texture> m_texture;
	  private:
		void SetupMesh();

		GLuint                   m_vao;
		GLuint                   m_vbo;
		bool                     m_initialized;
	};
} // namespace Engine