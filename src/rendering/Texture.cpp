#include "Texture.h"
#include "core/EngineData.h"
#define STB_IMAGE_IMPLEMENTATION

#include <spdlog/spdlog.h>
#include <stb/stb_image.h>

#include "rendering/Renderer.h"

#define DDS_USE_STD_FILESYSTEM 1

#include <filesystem>
#include <dds.hpp>
#include <EffekseerRendererGL/EffekseerRendererGL.GLExtension.h>

namespace Engine {
	std::unordered_set<GLuint> Engine::Texture::s_loadedTextures;

	Texture::Texture() : m_textureID(0), m_width(0), m_height(0), m_channels(0), m_isHDR(false)
	{
	}

    bool ends_with(std::string s, std::string end){
        if(s.length() < end.length()) return false;

        for(int i = 0; i < end.length(); i++){
            if(s[s.length() - 1 - i] != end[end.length() - 1 - i]){
                return false;
            }
        }

        return true;
    }

	bool Texture::LoadFromFile(const std::string& path)
	{
		// Check if file is HDR
		if (stbi_is_hdr(path.c_str())) {
			return LoadHDRFromFile(path);
		}
        if(ends_with(path, ".dds")){
            GetRenderer().log->info("TYRING TO LOAD DDS");
            return LoadDDSFromFile(path);
        }

		m_name = path.substr(path.find_last_of("/\\") + 1);
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
			GetRenderer().log->error("Failed to load texture: {}", path);
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
            GetRenderer().log->error("Unsupported number of channels: {}", m_channels);
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

    bool IsCompressedFormat(DXGI_FORMAT fmt)
    {
        switch (fmt)
        {
            case DXGI_FORMAT_BC1_UNORM:
            case DXGI_FORMAT_BC1_UNORM_SRGB:
            case DXGI_FORMAT_BC2_UNORM:
            case DXGI_FORMAT_BC2_UNORM_SRGB:
            case DXGI_FORMAT_BC3_UNORM:
            case DXGI_FORMAT_BC3_UNORM_SRGB:
            case DXGI_FORMAT_BC4_UNORM:
            case DXGI_FORMAT_BC4_SNORM:
            case DXGI_FORMAT_BC5_UNORM:
            case DXGI_FORMAT_BC5_SNORM:
            case DXGI_FORMAT_BC6H_UF16:
            case DXGI_FORMAT_BC6H_SF16:
            case DXGI_FORMAT_BC7_UNORM:
            case DXGI_FORMAT_BC7_UNORM_SRGB:
                return true;

            default:
                return false;
        }
    }

    uint32_t GetChannelCount(DXGI_FORMAT format)
    {
        switch (format)
        {
            // 1 channel
            case DXGI_FORMAT_R8_UNORM:
            case DXGI_FORMAT_R16_UNORM:
            case DXGI_FORMAT_R32_FLOAT:
            case DXGI_FORMAT_A8_UNORM:
                return 1;

                // 2 channels
            case DXGI_FORMAT_R8G8_UNORM:
            case DXGI_FORMAT_R16G16_UNORM:
            case DXGI_FORMAT_R32G32_FLOAT:
                return 2;

                // 3 channels
            case DXGI_FORMAT_R32G32B32_FLOAT:
                return 3;

                // 4 channels
            case DXGI_FORMAT_R8G8B8A8_UNORM:
            case DXGI_FORMAT_B8G8R8A8_UNORM:
            case DXGI_FORMAT_R16G16B16A16_FLOAT:
            case DXGI_FORMAT_R32G32B32A32_FLOAT:
                return 4;

            default:
                return 0; // compressed or unsupported
        }
    }

    struct GLFormat
    {
        GLenum internalFormat;
        GLenum format;
        GLenum type;
        bool compressed;
    };

    GLFormat GetGLFormat(DXGI_FORMAT fmt)
    {
        switch (fmt)
        {
            // ================================
            // 128-bit
            // ================================

            case DXGI_FORMAT_R32G32B32A32_FLOAT:
                return { GL_RGBA32F, GL_RGBA, GL_FLOAT, false };

            case DXGI_FORMAT_R32G32B32A32_UINT:
                return { GL_RGBA32UI, GL_RGBA_INTEGER, GL_UNSIGNED_INT, false };

            case DXGI_FORMAT_R32G32B32A32_SINT:
                return { GL_RGBA32I, GL_RGBA_INTEGER, GL_INT, false };

                // ================================
                // 96-bit
                // ================================

            case DXGI_FORMAT_R32G32B32_FLOAT:
                return { GL_RGB32F, GL_RGB, GL_FLOAT, false };

            case DXGI_FORMAT_R32G32B32_UINT:
                return { GL_RGB32UI, GL_RGB_INTEGER, GL_UNSIGNED_INT, false };

            case DXGI_FORMAT_R32G32B32_SINT:
                return { GL_RGB32I, GL_RGB_INTEGER, GL_INT, false };

                // ================================
                // 64-bit RGBA
                // ================================

            case DXGI_FORMAT_R16G16B16A16_FLOAT:
                return { GL_RGBA16F, GL_RGBA, GL_HALF_FLOAT, false };

            case DXGI_FORMAT_R16G16B16A16_UNORM:
                return { GL_RGBA16, GL_RGBA, GL_UNSIGNED_SHORT, false };

            case DXGI_FORMAT_R16G16B16A16_SNORM:
                return { GL_RGBA16_SNORM, GL_RGBA, GL_SHORT, false };

            case DXGI_FORMAT_R16G16B16A16_UINT:
                return { GL_RGBA16UI, GL_RGBA_INTEGER, GL_UNSIGNED_SHORT, false };

            case DXGI_FORMAT_R16G16B16A16_SINT:
                return { GL_RGBA16I, GL_RGBA_INTEGER, GL_SHORT, false };

                // ================================
                // 64-bit RG
                // ================================

            case DXGI_FORMAT_R32G32_FLOAT:
                return { GL_RG32F, GL_RG, GL_FLOAT, false };

            case DXGI_FORMAT_R32G32_UINT:
                return { GL_RG32UI, GL_RG_INTEGER, GL_UNSIGNED_INT, false };

            case DXGI_FORMAT_R32G32_SINT:
                return { GL_RG32I, GL_RG_INTEGER, GL_INT, false };

                // ================================
                // Depth / Stencil
                // ================================

            case DXGI_FORMAT_D32_FLOAT:
                return { GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT, false };

            case DXGI_FORMAT_D24_UNORM_S8_UINT:
                return { GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, false };

            case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
                return { GL_DEPTH32F_STENCIL8, GL_DEPTH_STENCIL, GL_FLOAT_32_UNSIGNED_INT_24_8_REV, false };

            case DXGI_FORMAT_D16_UNORM:
                return { GL_DEPTH_COMPONENT16, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, false };

                // ================================
                // 32-bit formats
                // ================================

            case DXGI_FORMAT_R10G10B10A2_UNORM:
                return { GL_RGB10_A2, GL_RGBA, GL_UNSIGNED_INT_2_10_10_10_REV, false };

            case DXGI_FORMAT_R10G10B10A2_UINT:
                return { GL_RGB10_A2UI, GL_RGBA_INTEGER, GL_UNSIGNED_INT_2_10_10_10_REV, false };

            case DXGI_FORMAT_R11G11B10_FLOAT:
                return { GL_R11F_G11F_B10F, GL_RGB, GL_UNSIGNED_INT_10F_11F_11F_REV, false };

            case DXGI_FORMAT_R8G8B8A8_UNORM:
                return { GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, false };

            case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
                return { GL_SRGB8_ALPHA8, GL_RGBA, GL_UNSIGNED_BYTE, false };

            case DXGI_FORMAT_R8G8B8A8_SNORM:
                return { GL_RGBA8_SNORM, GL_RGBA, GL_BYTE, false };

            case DXGI_FORMAT_R8G8B8A8_UINT:
                return { GL_RGBA8UI, GL_RGBA_INTEGER, GL_UNSIGNED_BYTE, false };

            case DXGI_FORMAT_R8G8B8A8_SINT:
                return { GL_RGBA8I, GL_RGBA_INTEGER, GL_BYTE, false };

            case DXGI_FORMAT_B8G8R8A8_UNORM:
                return { GL_RGBA8, GL_BGRA, GL_UNSIGNED_BYTE, false };

            case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
                return { GL_SRGB8_ALPHA8, GL_BGRA, GL_UNSIGNED_BYTE, false };

            case DXGI_FORMAT_B8G8R8X8_UNORM:
                return { GL_RGBA8, GL_BGRA, GL_UNSIGNED_BYTE, false };

                // ================================
                // RG
                // ================================

            case DXGI_FORMAT_R16G16_FLOAT:
                return { GL_RG16F, GL_RG, GL_HALF_FLOAT, false };

            case DXGI_FORMAT_R16G16_UNORM:
                return { GL_RG16, GL_RG, GL_UNSIGNED_SHORT, false };

            case DXGI_FORMAT_R16G16_SNORM:
                return { GL_RG16_SNORM, GL_RG, GL_SHORT, false };

            case DXGI_FORMAT_R16G16_UINT:
                return { GL_RG16UI, GL_RG_INTEGER, GL_UNSIGNED_SHORT, false };

            case DXGI_FORMAT_R16G16_SINT:
                return { GL_RG16I, GL_RG_INTEGER, GL_SHORT, false };

            case DXGI_FORMAT_R8G8_UNORM:
                return { GL_RG8, GL_RG, GL_UNSIGNED_BYTE, false };

            case DXGI_FORMAT_R8G8_SNORM:
                return { GL_RG8_SNORM, GL_RG, GL_BYTE, false };

            case DXGI_FORMAT_R8G8_UINT:
                return { GL_RG8UI, GL_RG_INTEGER, GL_UNSIGNED_BYTE, false };

            case DXGI_FORMAT_R8G8_SINT:
                return { GL_RG8I, GL_RG_INTEGER, GL_BYTE, false };

                // ================================
                // R
                // ================================

            case DXGI_FORMAT_R32_FLOAT:
                return { GL_R32F, GL_RED, GL_FLOAT, false };

            case DXGI_FORMAT_R32_UINT:
                return { GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, false };

            case DXGI_FORMAT_R32_SINT:
                return { GL_R32I, GL_RED_INTEGER, GL_INT, false };

            case DXGI_FORMAT_R16_FLOAT:
                return { GL_R16F, GL_RED, GL_HALF_FLOAT, false };

            case DXGI_FORMAT_R16_UNORM:
                return { GL_R16, GL_RED, GL_UNSIGNED_SHORT, false };

            case DXGI_FORMAT_R16_SNORM:
                return { GL_R16_SNORM, GL_RED, GL_SHORT, false };

            case DXGI_FORMAT_R16_UINT:
                return { GL_R16UI, GL_RED_INTEGER, GL_UNSIGNED_SHORT, false };

            case DXGI_FORMAT_R16_SINT:
                return { GL_R16I, GL_RED_INTEGER, GL_SHORT, false };

            case DXGI_FORMAT_R8_UNORM:
                return { GL_R8, GL_RED, GL_UNSIGNED_BYTE, false };

            case DXGI_FORMAT_R8_SNORM:
                return { GL_R8_SNORM, GL_RED, GL_BYTE, false };

            case DXGI_FORMAT_R8_UINT:
                return { GL_R8UI, GL_RED_INTEGER, GL_UNSIGNED_BYTE, false };

            case DXGI_FORMAT_R8_SINT:
                return { GL_R8I, GL_RED_INTEGER, GL_BYTE, false };

            case DXGI_FORMAT_A8_UNORM:
                return { GL_R8, GL_RED, GL_UNSIGNED_BYTE, false };

                // ================================
                // Packed legacy
                // ================================

            case DXGI_FORMAT_B5G6R5_UNORM:
                return { GL_RGB565, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, false };

            case DXGI_FORMAT_B5G5R5A1_UNORM:
                return { GL_RGB5_A1, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, false };

            case DXGI_FORMAT_B4G4R4A4_UNORM:
                return { GL_RGBA4, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, false };

                // ================================
                // BC Compressed
                // ================================

            case DXGI_FORMAT_BC1_UNORM:
                return { GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, 0, 0, true };

            case DXGI_FORMAT_BC1_UNORM_SRGB:
                return { GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT, 0, 0, true };

            case DXGI_FORMAT_BC2_UNORM:
                return { GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, 0, 0, true };

            case DXGI_FORMAT_BC2_UNORM_SRGB:
                return { GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT, 0, 0, true };

            case DXGI_FORMAT_BC3_UNORM:
                return { GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, 0, 0, true };

            case DXGI_FORMAT_BC3_UNORM_SRGB:
                return { GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT, 0, 0, true };

            case DXGI_FORMAT_BC4_UNORM:
                return { GL_COMPRESSED_RED_RGTC1, 0, 0, true };

            case DXGI_FORMAT_BC4_SNORM:
                return { GL_COMPRESSED_SIGNED_RED_RGTC1, 0, 0, true };

            case DXGI_FORMAT_BC5_UNORM:
                return { GL_COMPRESSED_RG_RGTC2, 0, 0, true };

            case DXGI_FORMAT_BC5_SNORM:
                return { GL_COMPRESSED_SIGNED_RG_RGTC2, 0, 0, true };

            case DXGI_FORMAT_BC6H_UF16:
                return { GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT, 0, 0, true };

            case DXGI_FORMAT_BC6H_SF16:
                return { GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT, 0, 0, true };

            case DXGI_FORMAT_BC7_UNORM:
                return { GL_COMPRESSED_RGBA_BPTC_UNORM, 0, 0, true };

            case DXGI_FORMAT_BC7_UNORM_SRGB:
                return { GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM, 0, 0, true };

            default:
                throw std::runtime_error("Unsupported or typeless DXGI format for OpenGL upload");
        }
    }

    GLuint UploadDDSTexture2D(const dds::Image* image)
    {
        GLuint tex;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        GLFormat glFmt = GetGLFormat(image->format);

        uint32_t width = image->width;
        uint32_t height = image->height;
        size_t offset = 0;

        for (uint32_t mip = 0; mip < image->numMips; ++mip)
        {
            if (glFmt.compressed)
            {
                size_t blockSize =
                        (image->format == DXGI_FORMAT_BC1_UNORM ||
                         image->format == DXGI_FORMAT_BC1_UNORM_SRGB ||
                         image->format == DXGI_FORMAT_BC4_UNORM ||
                         image->format == DXGI_FORMAT_BC4_SNORM) ? 8 : 16;

                size_t size =
                        ((width + 3) / 4) *
                        ((height + 3) / 4) *
                        blockSize;

                glCompressedTexImage2D(
                        GL_TEXTURE_2D,
                        mip,
                        glFmt.internalFormat,
                        width,
                        height,
                        0,
                        size,
                        image->data.get() + offset
                );

                offset += size;
            }
            else
            {
                glTexImage2D(
                        GL_TEXTURE_2D,
                        mip,
                        glFmt.internalFormat,
                        width,
                        height,
                        0,
                        glFmt.format,
                        glFmt.type,
                        image->data.get() + offset
                );

                // advance offset for uncompressed
                size_t bytesPerPixel = dds::getBitsPerPixel(image->format) / 8;
                offset += width * height * bytesPerPixel;
            }

            width  = std::max(1u, width  / 2);
            height = std::max(1u, height / 2);
        }

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                        image->numMips > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        return tex;
    }

    bool Texture::LoadDDSFromFile(const std::string &path) {
        m_name = path.substr(path.find_last_of("/\\") + 1);
        if (glGetString(GL_VERSION) == nullptr) {
            std::cerr << "No valid OpenGL context!" << std::endl;
        }




        // Load image data
        dds::Image image;
        auto result = dds::readFile(path, &image);

        if (result != dds::ReadResult::Success) {
            GetRenderer().log->error("Failed to load dds texture: {}", path);
            return false;
        }

        GetRenderer().log->info("loaded dds ({}x{}) with {} channels. Format: {}", m_width, m_height, m_channels, image.format);

        m_textureID = UploadDDSTexture2D(&image);
        s_loadedTextures.insert(m_textureID);

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
            GetRenderer().log->error("No data for HDR texture: {}", path);
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
		GetDefaultLogger()->info("Loaded HDR texture: {} ({}x{}, {} channels)", path, m_width, m_height, m_channels);
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
		GetDefaultLogger()->info("Cleaned up all tracked textures.");
	}



} // namespace Engine