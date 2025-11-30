#include <glad/glad.h>
#include "RmlUiBackend.h"
#include "core/EngineData.h"
#include "rendering/ui/GameUIManager.h"
#include "rendering/ui/GameUIManager.h"
#include <RmlUi/Core.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// stb_image is already used elsewhere in the project
#include <stb/stb_image.h>

namespace Engine {

// Vertex shader source
static const char* vertex_shader_source = R"(
#version 330 core
layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inTexCoord;

uniform mat4 uProjection;
uniform mat4 uTransform;
uniform vec2 uTranslation;

out vec4 fragColor;
out vec2 fragTexCoord;

void main() {
    vec4 pos = vec4(inPosition + uTranslation, 0.0, 1.0);
    gl_Position = uProjection * uTransform * pos;
    fragColor = inColor;
    fragTexCoord = inTexCoord;
}
)";

// Fragment shader source
static const char* fragment_shader_source = R"(
#version 330 core
in vec4 fragColor;
in vec2 fragTexCoord;

uniform sampler2D uTexture;
uniform bool uHasTexture;

out vec4 outColor;

void main() {
    if (uHasTexture) {
        outColor = fragColor * texture(uTexture, fragTexCoord);
    } else {
        outColor = fragColor;
    }
}
)";

// Helper function to compile shader
static GLuint CompileGLShader(GLenum type, const char* source) {
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &source, nullptr);
	glCompileShader(shader);

	GLint success;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		char info_log[512];
		glGetShaderInfoLog(shader, 512, nullptr, info_log);
		GetGameUIManager().log->error("RmlUi Shader compilation failed: {}", info_log);
		return 0;
	}

	return shader;
}

// ============================================================================
// RmlUi_RenderInterface_GL3
// ============================================================================

RmlUi_RenderInterface_GL3::RmlUi_RenderInterface_GL3() {
	CreateShaders();
}

RmlUi_RenderInterface_GL3::~RmlUi_RenderInterface_GL3() {
	DestroyShaders();
}

void RmlUi_RenderInterface_GL3::CreateShaders() {
	// Compile shaders
	GLuint vertex_shader = CompileGLShader(GL_VERTEX_SHADER, vertex_shader_source);
	GLuint fragment_shader = CompileGLShader(GL_FRAGMENT_SHADER, fragment_shader_source);

	if (!vertex_shader || !fragment_shader) {
		GetGameUIManager().log->error("Failed to compile RmlUi shaders");
		return;
	}

	// Link program
	m_program_id = glCreateProgram();
	glAttachShader(m_program_id, vertex_shader);
	glAttachShader(m_program_id, fragment_shader);
	glLinkProgram(m_program_id);

	GLint success;
	glGetProgramiv(m_program_id, GL_LINK_STATUS, &success);
	if (!success) {
		char info_log[512];
		glGetProgramInfoLog(m_program_id, 512, nullptr, info_log);
		GetGameUIManager().log->error("RmlUi Program linking failed: {}", info_log);
	}

	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);

	// Set identity transform initially
	m_transform = glm::mat4(1.0f);

	GetGameUIManager().log->info("RmlUi OpenGL3 backend initialized");
}

void RmlUi_RenderInterface_GL3::DestroyShaders() {
	if (m_program_id) {
		glDeleteProgram(m_program_id);
	}

	// Clean up geometry
	for (auto& [handle, geom] : m_geometry) {
		if (geom.vao) glDeleteVertexArrays(1, &geom.vao);
		if (geom.vbo) glDeleteBuffers(1, &geom.vbo);
		if (geom.ibo) glDeleteBuffers(1, &geom.ibo);
	}
	m_geometry.clear();

	// Clean up textures
	for (auto& [handle, gl_texture] : m_textures) {
		glDeleteTextures(1, &gl_texture);
	}
	m_textures.clear();
}

void RmlUi_RenderInterface_GL3::SetViewport(int width, int height) {
	m_viewport_width = width;
	m_viewport_height = height;

	// Create orthographic projection matrix
	m_projection = glm::ortho(0.0f, static_cast<float>(width), 
	                          static_cast<float>(height), 0.0f, 
	                          -1.0f, 1.0f);
}

Rml::CompiledGeometryHandle RmlUi_RenderInterface_GL3::CompileGeometry(
    Rml::Span<const Rml::Vertex> vertices,
    Rml::Span<const int> indices) {
	
	GeometryData geom;
	geom.num_indices = static_cast<int>(indices.size());

	// Create VAO, VBO, IBO
	glGenVertexArrays(1, &geom.vao);
	glGenBuffers(1, &geom.vbo);
	glGenBuffers(1, &geom.ibo);

	glBindVertexArray(geom.vao);
	
	glBindBuffer(GL_ARRAY_BUFFER, geom.vbo);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Rml::Vertex), 
	             vertices.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geom.ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(int), 
	             indices.data(), GL_STATIC_DRAW);

	// Position attribute
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Rml::Vertex),
	                      (void*)offsetof(Rml::Vertex, position));

	// Color attribute
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Rml::Vertex),
	                      (void*)offsetof(Rml::Vertex, colour));

	// TexCoord attribute
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Rml::Vertex),
	                      (void*)offsetof(Rml::Vertex, tex_coord));

	glBindVertexArray(0);

	// Store geometry
	Rml::CompiledGeometryHandle handle = m_next_geometry_id++;
	m_geometry[handle] = geom;

	return handle;
}

void RmlUi_RenderInterface_GL3::RenderGeometry(Rml::CompiledGeometryHandle geometry,
                                                Rml::Vector2f translation,
                                                Rml::TextureHandle texture) {
	auto it = m_geometry.find(geometry);
	if (it == m_geometry.end()) {
		return;
	}

	const GeometryData& geom = it->second;

	// Use our shader program
	glUseProgram(m_program_id);

	// Set uniforms
	GLint proj_loc = glGetUniformLocation(m_program_id, "uProjection");
	GLint transform_loc = glGetUniformLocation(m_program_id, "uTransform");
	GLint translation_loc = glGetUniformLocation(m_program_id, "uTranslation");
	GLint texture_loc = glGetUniformLocation(m_program_id, "uTexture");
	GLint has_texture_loc = glGetUniformLocation(m_program_id, "uHasTexture");

	glUniformMatrix4fv(proj_loc, 1, GL_FALSE, glm::value_ptr(m_projection));
	glUniformMatrix4fv(transform_loc, 1, GL_FALSE, glm::value_ptr(m_transform));
	glUniform2f(translation_loc, translation.x, translation.y);
	glUniform1i(texture_loc, 0);

	// Bind texture if available
	bool has_texture = (texture != 0);
	glUniform1i(has_texture_loc, has_texture);

	if (has_texture) {
		auto tex_it = m_textures.find(texture);
		if (tex_it != m_textures.end()) {
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, tex_it->second);
		}
	}

	// Draw
	glBindVertexArray(geom.vao);
	glDrawElements(GL_TRIANGLES, geom.num_indices, GL_UNSIGNED_INT, nullptr);
	glBindVertexArray(0);
	
	glUseProgram(0);
}

void RmlUi_RenderInterface_GL3::ReleaseGeometry(Rml::CompiledGeometryHandle geometry) {
	auto it = m_geometry.find(geometry);
	if (it != m_geometry.end()) {
		GeometryData& geom = it->second;
		if (geom.vao) glDeleteVertexArrays(1, &geom.vao);
		if (geom.vbo) glDeleteBuffers(1, &geom.vbo);
		if (geom.ibo) glDeleteBuffers(1, &geom.ibo);
		m_geometry.erase(it);
	}
}

void RmlUi_RenderInterface_GL3::EnableScissorRegion(bool enable) {
	if (enable) {
		glEnable(GL_SCISSOR_TEST);
	} else {
		glDisable(GL_SCISSOR_TEST);
	}
}

void RmlUi_RenderInterface_GL3::SetScissorRegion(Rml::Rectanglei region) {
	// OpenGL's scissor origin is bottom-left, but RmlUi uses top-left
	glScissor(region.Left(), m_viewport_height - (region.Top() + region.Height()), 
	          region.Width(), region.Height());
}

Rml::TextureHandle RmlUi_RenderInterface_GL3::LoadTexture(Rml::Vector2i& texture_dimensions,
                                             const Rml::String& source) {
	int width, height, channels;
	unsigned char* data = stbi_load(source.c_str(), &width, &height, &channels, 4);

	if (!data) {
		GetGameUIManager().log->error("Failed to load texture: {}", source);
		return 0;
	}

	texture_dimensions.x = width;
	texture_dimensions.y = height;

	// Generate texture
	GLuint gl_texture;
	glGenTextures(1, &gl_texture);
	glBindTexture(GL_TEXTURE_2D, gl_texture);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	stbi_image_free(data);

	// Store texture
	Rml::TextureHandle handle = m_next_texture_id++;
	m_textures[handle] = gl_texture;

	GetGameUIManager().log->debug("Loaded RmlUi texture: {} ({}x{})", source, width, height);

	return handle;
}

Rml::TextureHandle RmlUi_RenderInterface_GL3::GenerateTexture(Rml::Span<const Rml::byte> source,
                                                 Rml::Vector2i source_dimensions) {
	// Generate texture from raw pixel data
	GLuint gl_texture;
	glGenTextures(1, &gl_texture);
	glBindTexture(GL_TEXTURE_2D, gl_texture);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, source_dimensions.x, source_dimensions.y, 
	             0, GL_RGBA, GL_UNSIGNED_BYTE, source.data());

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Store texture
	Rml::TextureHandle handle = m_next_texture_id++;
	m_textures[handle] = gl_texture;

	return handle;
}

void RmlUi_RenderInterface_GL3::ReleaseTexture(Rml::TextureHandle texture) {
	auto it = m_textures.find(texture);
	if (it != m_textures.end()) {
		glDeleteTextures(1, &it->second);
		m_textures.erase(it);
	}
}

void RmlUi_RenderInterface_GL3::SetTransform(const Rml::Matrix4f* transform) {
	if (transform) {
		// Convert RmlUi matrix to GLM matrix
		m_transform = glm::make_mat4(transform->data());
	} else {
		// Reset to identity
		m_transform = glm::mat4(1.0f);
	}
}

// ============================================================================
// RmlUi_SystemInterface
// ============================================================================

RmlUi_SystemInterface::RmlUi_SystemInterface() {
	m_start_time = glfwGetTime();
}

double RmlUi_SystemInterface::GetElapsedTime() {
	return glfwGetTime() - m_start_time;
}

} // namespace Engine
