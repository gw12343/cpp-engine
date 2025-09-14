#pragma once

#include "renderer.h"

#include <glm/glm.hpp>
#include <vector>
#define GLFW_INCLUDE_NONE
#include "Camera.h"
#include "animation/renderer.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "imgui.h"
#include "ozz/base/containers/vector.h"
#include "ozz/base/log.h"
#include "ozz/base/memory/unique_ptr.h"
#include "rendering/Material.h"

#include "entt/entt.hpp"
#include "rendering/Shader.h"


// Provides helper macro to test for glGetError on a gl call.
#ifndef NDEBUG
//     gl##_f;
#define GL(_f)                                                                                                                                                                                                                                 \
	do {                                                                                                                                                                                                                                       \
		gl##_f;                                                                                                                                                                                                                                \
		GLenum gl_err = glGetError();                                                                                                                                                                                                          \
		if (gl_err != 0) {                                                                                                                                                                                                                     \
			ozz::log::Err() << "GL error 0x" << std::hex << gl_err << " returned from 'gl" << #_f << std::endl;                                                                                                                                \
		}                                                                                                                                                                                                                                      \
		assert(gl_err == GL_NO_ERROR);                                                                                                                                                                                                         \
	} while (void(0), 0)

#else // NDEBUG
#define GL(_f) gl##_f
#endif // NDEBUG

// Convenient macro definition for specifying buffer offsets.
#define GL_PTR_OFFSET(i) reinterpret_cast<void*>(static_cast<intptr_t>(i))

namespace ozz {
	namespace animation {
		class Skeleton;
	}
	namespace math {
		struct Float4x4;
	}
} // namespace ozz

class AnimationShader;
class PointsShader;
class SkeletonShader;
class AmbientShader;
class AmbientTexturedShader;
class AmbientShaderInstanced;
class GlImmediateRenderer;


typedef unsigned int GLuint;
typedef unsigned int GLenum;
typedef int          GLsizei;


// Implements Renderer interface.
class RendererImpl : public AnimatedRenderer {
  public:
	RendererImpl();
	~RendererImpl() override;

	// See Renderer for all the details about the API.
	bool Initialize() override;

	bool DrawAxes(const ozz::math::Float4x4& _transform) override;

	bool DrawGrid(int _cell_count, float _cell_size) override;

	bool DrawSkeleton(const ozz::animation::Skeleton& _skeleton, const ozz::math::Float4x4& _transform, bool _draw_joints) override;

	bool DrawPosture(const ozz::animation::Skeleton& _skeleton, ozz::span<const ozz::math::Float4x4> _matrices, const ozz::math::Float4x4& _transform, bool _draw_joints) override;

	bool DrawPoints(const ozz::span<const float>& _positions, const ozz::span<const float>& _sizes, const ozz::span<const Engine::Color>& _colors, const ozz::math::Float4x4& _transform, bool _screen_space) override;

	bool DrawBoxIm(const ozz::math::Box& _box, const ozz::math::Float4x4& _transform, const Engine::Color& _color) override;

	bool DrawBoxIm(const ozz::math::Box& _box, const ozz::math::Float4x4& _transform, const Engine::Color _colors[2]) override;

	bool DrawBoxShaded(const ozz::math::Box& _box, ozz::span<const ozz::math::Float4x4> _transforms, const Engine::Color& _color) override;

	bool DrawSphereIm(float _radius, const ozz::math::Float4x4& _transform, const Engine::Color& _color) override;

	bool DrawSphereShaded(float _radius, ozz::span<const ozz::math::Float4x4> _transforms, const Engine::Color& _color) override;

	virtual bool DrawSkinnedMesh(const Engine::Mesh& _mesh, ozz::span<ozz::math::Float4x4> _skinning_matrices, const ozz::math::Float4x4& _transform, AssetHandle<Material> _material, const Options& _options = Options());
	bool         DrawSkinnedMeshMousePicking(glm::vec3 entityColor, const Engine::Mesh& _mesh, ozz::span<ozz::math::Float4x4> _skinning_matrices, const ozz::math::Float4x4& _transform);
	bool         DrawSkinnedMeshShadows(Engine::Shader* shadowShader, const Engine::Mesh& _mesh, ozz::span<ozz::math::Float4x4> _skinning_matrices, const ozz::math::Float4x4& _transform);

	virtual bool DrawMesh(const Engine::Mesh& _mesh, const ozz::math::Float4x4& _transform, AssetHandle<Material> _material, const Options& _options = Options());

	bool DrawLines(ozz::span<const ozz::math::Float3> _vertices, const Engine::Color& _color, const ozz::math::Float4x4& _transform) override;

	bool DrawLineStrip(ozz::span<const ozz::math::Float3> _vertices, const Engine::Color& _color, const ozz::math::Float4x4& _transform) override;

	bool DrawVectors(ozz::span<const float>     _positions,
	                 size_t                     _positions_stride,
	                 ozz::span<const float>     _directions,
	                 size_t                     _directions_stride,
	                 int                        _num_vectors,
	                 float                      _vector_length,
	                 const Engine::Color&       _color,
	                 const ozz::math::Float4x4& _transform) override;

	bool DrawBinormals(ozz::span<const float>     _positions,
	                   size_t                     _positions_stride,
	                   ozz::span<const float>     _normals,
	                   size_t                     _normals_stride,
	                   ozz::span<const float>     _tangents,
	                   size_t                     _tangents_stride,
	                   ozz::span<const float>     _handenesses,
	                   size_t                     _handenesses_stride,
	                   int                        _num_vectors,
	                   float                      _vector_length,
	                   const Engine::Color&       _color,
	                   const ozz::math::Float4x4& _transform) override;

	// Get GL immediate renderer implementation;
	[[nodiscard]] GlImmediateRenderer* immediate_renderer() const { return immediate_.get(); }

  private:
	// Defines the internal structure used to define a model.
	struct Model {
		Model();
		~Model();

		GLuint                          vbo;
		GLenum                          mode;
		GLsizei                         count;
		ozz::unique_ptr<SkeletonShader> shader;
	};

	// Detects and initializes all OpenGL extension.
	// Return true if all mandatory extensions were found.
	bool InitOpenGLExtensions();

	// Initializes posture rendering.
	// Return true if initialization succeeded.
	bool InitPostureRendering();

	// Initializes the checkered texture.
	// Return true if initialization succeeded.
	// bool InitCheckeredTexture();

	// Draw posture internal non-instanced rendering fall back implementation.
	void DrawPosture_Impl(const ozz::math::Float4x4& _transform, const float* _uniforms, int _instance_count, bool _draw_joints);

	// Draw posture internal instanced rendering implementation.
	void DrawPosture_InstancedImpl(const ozz::math::Float4x4& _transform, const float* _uniforms, int _instance_count, bool _draw_joints);

	// Array of matrices used to store model space matrices during DrawSkeleton
	// execution.
	ozz::vector<ozz::math::Float4x4> prealloc_models_;


	// Bone and joint model objects.
	Model models_[2];

#ifndef EMSCRIPTEN
	// Vertex array
	GLuint vertex_array_o_ = 0;
#endif // EMSCRIPTEN

	// Dynamic vbo used for arrays.
	GLuint dynamic_array_bo_ = 0;

	// Dynamic vbo used for indices.
	GLuint dynamic_index_bo_ = 0;

	// Volatile memory buffer that can be used within function scope.
	// Minimum alignment is 16 bytes.
	class ScratchBuffer {
	  public:
		ScratchBuffer();
		~ScratchBuffer();

		// Resizes the buffer to the new size and return the memory address.
		void* Resize(size_t _size);

	  private:
		void*  buffer_;
		size_t size_;
	};
	ScratchBuffer scratch_buffer_;

	// Immediate renderer implementation.
	ozz::unique_ptr<GlImmediateRenderer> immediate_;

	// Ambient rendering shader.
	ozz::unique_ptr<AmbientShader>          ambient_shader;
	ozz::unique_ptr<AmbientTexturedShader>  ambient_textured_shader;
	ozz::unique_ptr<AmbientShaderInstanced> ambient_shader_instanced;
	ozz::unique_ptr<PointsShader>           points_shader;
	Engine::Shader                          m_animation_mouse_picking_shader;

	// Checkered texture
	// GLuint checkered_texture_ = 0;
};