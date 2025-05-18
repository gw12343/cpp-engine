#pragma once

#include <string>
#include <glad/glad.h>
#include <spdlog/spdlog.h>

class Texture {
public:
    Texture();
    ~Texture();

    bool LoadFromFile(const std::string& path);
    bool LoadHDRFromFile(const std::string& path);
    void Bind(unsigned int unit = 0) const;
    void Unbind() const;

    GLuint GetID() const { return m_textureID; }
    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }
    bool IsHDR() const { return m_isHDR; }

private:
    GLuint m_textureID;
    int m_width;
    int m_height;
    int m_channels;
    bool m_isHDR;
}; 
