#include "Texture.h"
#define STB_IMAGE_IMPLEMENTATION
#include <iostream>
#include <spdlog/spdlog.h>
#include <stb/stb_image.h>
#include <glad/glad.h>

namespace Engine {
	std::unordered_set<GLuint> Engine::Texture::s_loadedTextures;

	Texture::Texture() : m_textureID(0), m_width(0), m_height(0), m_channels(0), m_isHDR(false)
	{
	}

	bool Texture::LoadFromFile(const std::string& path)
	{
		// Check if file is HDR
		if (stbi_is_hdr(path.c_str())) {
			return LoadHDRFromFile(path);
		}
		m_name = path.substr(path.find_last_of("/\\") + 1);
		SPDLOG_DEBUG("TRYING TO LOAD: {}", path);
		if (glGetString(GL_VERSION) == nullptr) {
			std::cerr << "No valid OpenGL context!" << std::endl;
		}

		// Generate texture ID
		glGenTextures(1, &m_textureID);
		s_loadedTextures.insert(m_textureID);

		GLenum err = glGetError();
		if (err != GL_NO_ERROR) {
			std::cerr << "OpenGL Error: " << err << std::endl;
		}

		glBindTexture(GL_TEXTURE_2D, m_textureID);


		// Set texture parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// Load image data
		stbi_set_flip_vertically_on_load(false);
		unsigned char* data = stbi_load(path.c_str(), &m_width, &m_height, &m_channels, 0);

		if (!data) {
			spdlog::error("Failed to load texture: {}", path);
			return false;
		}

		// Determine format based on number of channels
		GLenum format;
		if (m_channels == 1)
			format = GL_RED;
		else if (m_channels == 3)
			format = GL_RGB;
		else if (m_channels == 4)
			format = GL_RGBA;
		else {
			spdlog::error("Unsupported number of channels: {}", m_channels);
			stbi_image_free(data);
			return false;
		}

		// Upload texture data
		glTexImage2D(GL_TEXTURE_2D, 0, static_cast<int>(format), m_width, m_height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		// Free image data
		stbi_image_free(data);
		glBindTexture(GL_TEXTURE_2D, 0);

		m_isHDR = false;
		return true;
	}

	bool Texture::LoadHDRFromFile(const std::string& path)
	{
		m_name = path.substr(path.find_last_of("/\\") + 1);
		// Generate texture ID
		glGenTextures(1, &m_textureID);
		s_loadedTextures.insert(m_textureID);
		glBindTexture(GL_TEXTURE_2D, m_textureID);

		// Set texture parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// Load HDR image data
		stbi_set_flip_vertically_on_load(true);
		float* data = stbi_loadf(path.c_str(), &m_width, &m_height, &m_channels, 0);

		if (!data) {
			spdlog::error("No data for HDR texture: {}", path);
			return false;
		}

		// Determine format based on number of channels
		GLenum internalFormat, format;
		if (m_channels == 1) {
			internalFormat = GL_R16F;
			format         = GL_RED;
		}
		else if (m_channels == 3) {
			internalFormat = GL_RGB16F;
			format         = GL_RGB;
		}
		else if (m_channels == 4) {
			internalFormat = GL_RGBA16F;
			format         = GL_RGBA;
		}
		else {
			spdlog::error("Unsupported number of channels for HDR: {}", m_channels);
			stbi_image_free(data);
			return false;
		}

		// Upload HDR texture data
		glTexImage2D(GL_TEXTURE_2D, 0, static_cast<int>(internalFormat), m_width, m_height, 0, format, GL_FLOAT, data);

		// Free image data
		stbi_image_free(data);
		glBindTexture(GL_TEXTURE_2D, 0);

		m_isHDR = true;
		SPDLOG_INFO("Loaded HDR texture: {} ({}x{}, {} channels)", path, m_width, m_height, m_channels);
		return true;
	}

	void Texture::Bind(unsigned int unit) const
	{
		glActiveTexture(GL_TEXTURE0 + unit);
		glBindTexture(GL_TEXTURE_2D, m_textureID);
	}

	void Texture::Unbind()
	{
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	[[maybe_unused]] void Engine::Texture::CleanUp()
	{
		if (m_textureID != 0) {
			glDeleteTextures(1, &m_textureID);
			s_loadedTextures.erase(m_textureID);
			m_textureID = 0;
		}
	}

	void Texture::CleanAllTextures()
	{
		for (GLuint texID : s_loadedTextures) {
			glDeleteTextures(1, &texID);
		}
		s_loadedTextures.clear();
		SPDLOG_INFO("Cleaned up all tracked textures.");
	}

} // namespace Engine