#include "Skybox.h"

#include <spdlog/spdlog.h>
#include <glad/glad.h>

namespace Engine {
	Skybox::Skybox() : m_vao(0), m_vbo(0), m_texture(nullptr), m_initialized(false)
	{
	}

	Skybox::~Skybox()
	{
		if (m_initialized) {
			glDeleteVertexArrays(1, &m_vao);
			glDeleteBuffers(1, &m_vbo);
		}
	}

	bool Skybox::LoadFromFile(const std::string& path)
	{
		// Create texture for skybox
		m_texture = std::make_unique<Texture>();
		if (!m_texture->LoadHDRFromFile(path)) {
			spdlog::error("Failed to load skybox texture: {}", path);
			return false;
		}

		SetupMesh();
		m_initialized = true;
		return true;
	}

	void Skybox::SetupMesh()
	{
		// Skybox vertices (cube)
		float skyboxVertices[] = {// positions
		                          -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f,

		                          -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,

		                          1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f,

		                          -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,

		                          -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f,

		                          -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f};

		// Create VAO and VBO
		glGenVertexArrays(1, &m_vao);
		glGenBuffers(1, &m_vbo);

		glBindVertexArray(m_vao);
		glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);

		// Position attribute
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*) 0);

		glBindVertexArray(0);
	}

	void Skybox::Draw(const Shader& shader) const
	{
		if (!m_initialized) return;

		// Save current OpenGL state
		GLboolean depthMask;
		glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);

		// Draw skybox
		glDepthFunc(GL_LEQUAL); // Change depth function so depth test passes when values are equal to depth buffer's content
		glDepthMask(GL_FALSE);  // Don't write to depth buffer

		// Bind texture
		m_texture->Bind(0);
		shader.SetInt("skybox", 0);

		// Draw mesh
		glBindVertexArray(m_vao);
		glDrawArrays(GL_TRIANGLES, 0, 36);
		glBindVertexArray(0);

		// Restore OpenGL state
		glDepthMask(depthMask);
		glDepthFunc(GL_LESS); // Set depth function back to default
	}
} // namespace Engine