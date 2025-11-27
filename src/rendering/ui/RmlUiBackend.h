#pragma once

#include <RmlUi/Core/RenderInterface.h>
#include <RmlUi/Core/SystemInterface.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <unordered_map>
#include <vector>

namespace Engine {

/**
 * @brief OpenGL3 Render Interface for RmlUi
 * 
 * Implements the RmlUi rendering interface using OpenGL 3.3+ calls.
 * Based on the reference SDL_GL3 backend from RmlUi samples.
 */
class RmlUi_RenderInterface_GL3 : public Rml::RenderInterface {
public:
	RmlUi_RenderInterface_GL3();
	~RmlUi_RenderInterface_GL3() override;

	// Viewport management
	void SetViewport(int width, int height);

	// Geometry compilation and rendering
	Rml::CompiledGeometryHandle CompileGeometry(Rml::Span<const Rml::Vertex> vertices, 
	                                            Rml::Span<const int> indices) override;
	void RenderGeometry(Rml::CompiledGeometryHandle geometry, 
	                    Rml::Vector2f translation, 
	                    Rml::TextureHandle texture) override;
	void ReleaseGeometry(Rml::CompiledGeometryHandle geometry) override;

	// Scissor region (clipping)
	void EnableScissorRegion(bool enable) override;
	void SetScissorRegion(Rml::Rectanglei region) override;

	// Texture management
	Rml::TextureHandle LoadTexture(Rml::Vector2i& texture_dimensions,
	                                const Rml::String& source) override;
	Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte> source,
	                                    Rml::Vector2i source_dimensions) override;
	void ReleaseTexture(Rml::TextureHandle texture) override;

	// Transform
	void SetTransform(const Rml::Matrix4f* transform) override;

private:
	struct GeometryData {
		GLuint vao = 0;
		GLuint vbo = 0;
		GLuint ibo = 0;
		int num_indices = 0;
	};

	// Shader programs
	void CreateShaders();
	void DestroyShaders();

	// Rendering state
	GLuint m_program_id = 0;
	int m_viewport_width = 0;
	int m_viewport_height = 0;
	
	// Projection matrix
	glm::mat4 m_projection;
	glm::mat4 m_transform;
	
	// Geometry and texture tracking
	Rml::CompiledGeometryHandle m_next_geometry_id = 1;
	Rml::TextureHandle m_next_texture_id = 1;
	std::unordered_map<Rml::CompiledGeometryHandle, GeometryData> m_geometry;
	std::unordered_map<Rml::TextureHandle, GLuint> m_textures;
};

/**
 * @brief System Interface for RmlUi
 * 
 * Provides system-level functions like timing.
 */
class RmlUi_SystemInterface : public Rml::SystemInterface {
public:
	RmlUi_SystemInterface();
	~RmlUi_SystemInterface() override = default;

	// Timing
	double GetElapsedTime() override;

private:
	double m_start_time;
};

} // namespace Engine
