#pragma once


#include <spdlog/spdlog.h>
#include <string>
#include <unordered_set>

typedef unsigned int GLuint;

namespace Engine {
	class Texture {
	  public:
		Texture();
		~Texture() = default;

		bool                  LoadFromFile(const std::string& path);
		bool                  LoadHDRFromFile(const std::string& path);
		void                  Bind(unsigned int unit = 0) const;
		[[maybe_unused]] void CleanUp();
		static void           CleanAllTextures();
		static void           Unbind();

		std::string                         GetName() const { return m_name; }
		[[nodiscard]] GLuint                GetID() const { return m_textureID; }
		[[nodiscard]] int                   GetWidth() const { return m_width; }
		[[nodiscard]] int                   GetHeight() const { return m_height; }
		[[maybe_unused]] [[nodiscard]] bool IsHDR() const { return m_isHDR; }

	  private:
		GLuint      m_textureID;
		int         m_width;
		int         m_height;
		int         m_channels;
		bool        m_isHDR;
		std::string m_name;

		static std::unordered_set<GLuint> s_loadedTextures;
	};
} // namespace Engine
