#include "ozz/base/memory/unique_ptr.h"
#include "renderer_impl.h"


namespace ozz::math {
	struct Float4x4;
}


// Declares a shader program.
class AnimationShader {
  public:
	// Construct a fixed function pipeline shader. Bind Shader::Build to specify
	// shader sources.
	AnimationShader();

	// Destruct a shader.
	virtual ~AnimationShader();

	// Returns the shader program that can be bound to the OpenGL context.
	[[nodiscard]] GLuint program() const { return program_; }

	// Request an uniform location and pushes it to the uniform stack.
	// The uniform location is then accessible thought uniform().
	bool BindUniform(const char* _semantic);

	// Get an uniform location from the stack at index _index.
	[[nodiscard]] GLint uniform(int _index) const { return uniforms_[_index]; }

	// Request an attribute location and pushes it to the uniform stack.
	// The varying location is then accessible thought attrib().
	bool FindAttrib(const char* _semantic);

	// Get an varying location from the stack at index _index.
	[[nodiscard]] GLint attrib(int _index) const { return attribs_[_index]; }

	// Unblind shader.
	virtual void Unbind();

  protected:
	// Constructs a shader from _vertex and _fragment glsl sources.
	// Mutliple source files can be specified using the *count argument.
	bool BuildFromSource(int _vertex_count, const char** _vertex, int _fragment_count, const char** _fragment);

  private:
	// Unbind all attribs from GL.
	void UnbindAttribs();

	// Shader program
	GLuint program_;

	// Vertex and fragment shaders
	GLuint vertex_;
	GLuint fragment_;

	// Uniform locations, in the order they were requested.
	ozz::vector<GLint> uniforms_;

	// Varying locations, in the order they were requested.
	ozz::vector<GLint> attribs_;
};

class ImmediatePCShader : public AnimationShader {
  public:
	ImmediatePCShader()           = default;
	~ImmediatePCShader() override = default;

	// Constructs the shader.
	// Returns nullptr if shader compilation failed or a valid Shader pointer on
	// success. The shader must then be deleted using default allocator Delete
	// function.
	static ozz::unique_ptr<ImmediatePCShader> Build();

	// Binds the shader.
	void Bind(const ozz::math::Float4x4& _model, const ozz::math::Float4x4& _view_proj, GLsizei _pos_stride, GLsizei _pos_offset, GLsizei _color_stride, GLsizei _color_offset);
};

class ImmediatePTCShader : public AnimationShader {
  public:
	ImmediatePTCShader()           = default;
	~ImmediatePTCShader() override = default;

	// Constructs the shader.
	// Returns nullptr if shader compilation failed or a valid Shader pointer on
	// success. The shader must then be deleted using default allocator Delete
	// function.
	static ozz::unique_ptr<ImmediatePTCShader> Build();

	// Binds the shader.
	void Bind(const ozz::math::Float4x4& _model, const ozz::math::Float4x4& _view_proj, GLsizei _pos_stride, GLsizei _pos_offset, GLsizei _tex_stride, GLsizei _tex_offset, GLsizei _color_stride, GLsizei _color_offset);
};

class PointsShader : public AnimationShader {
  public:
	PointsShader()           = default;
	~PointsShader() override = default;

	// Constructs the shader.
	// Returns nullptr if shader compilation failed or a valid Shader pointer on
	// success. The shader must then be deleted using default allocator Delete
	// function.
	static ozz::unique_ptr<PointsShader> Build();

	// Binds the shader.
	struct GenericAttrib {
		GLint color;
		GLint size;
	};
	GenericAttrib
	Bind(const ozz::math::Float4x4& _model, const ozz::math::Float4x4& _view_proj, GLsizei _pos_stride, GLsizei _pos_offset, GLsizei _color_stride, GLsizei _color_offset, GLsizei _size_stride, GLsizei _size_offset, bool _screen_space);
};

class SkeletonShader : public AnimationShader {
  public:
	SkeletonShader()           = default;
	~SkeletonShader() override = default;

	// Binds the shader.
	void Bind(const ozz::math::Float4x4& _model, const ozz::math::Float4x4& _view_proj, GLsizei _pos_stride, GLsizei _pos_offset, GLsizei _normal_stride, GLsizei _normal_offset, GLsizei _color_stride, GLsizei _color_offset);

	// Get an attribute location for the join, in cased of instanced rendering.
	[[nodiscard]] GLint joint_instanced_attrib() const { return attrib(3); }

	// Get an uniform location for the join, in cased of non-instanced rendering.
	[[nodiscard]] GLint joint_uniform() const { return uniform(2); }
};

class JointShader : public SkeletonShader {
  public:
	JointShader()           = default;
	~JointShader() override = default;

	// Constructs the shader.
	// Returns nullptr if shader compilation failed or a valid Shader pointer on
	// success. The shader must then be deleted using default allocator Delete
	// function.
	static ozz::unique_ptr<JointShader> Build();
};

class BoneShader : public SkeletonShader {
  public:
	BoneShader()           = default;
	~BoneShader() override = default;

	// Constructs the shader.
	// Returns nullptr if shader compilation failed or a valid Shader pointer on
	// success. The shader must then be deleted using default allocator Delete
	// function.
	static ozz::unique_ptr<BoneShader> Build();
};

class AmbientShader : public AnimationShader {
  public:
	AmbientShader()           = default;
	~AmbientShader() override = default;

	// Constructs the shader.
	// Returns nullptr if shader compilation failed or a valid Shader pointer on
	// success. The shader must then be deleted using default allocator Delete
	// function.
	static ozz::unique_ptr<AmbientShader> Build();

	// Binds the shader.
	void
	Bind(const ozz::math::Float4x4& _model, const ozz::math::Float4x4& _view_proj, GLsizei _pos_stride, GLsizei _pos_offset, GLsizei _normal_stride, GLsizei _normal_offset, GLsizei _color_stride, GLsizei _color_offset, bool _color_float);

  protected:
	bool InternalBuild(int _vertex_count, const char** _vertex, int _fragment_count, const char** _fragment);
};

class AmbientShaderInstanced : public AnimationShader {
  public:
	AmbientShaderInstanced()           = default;
	~AmbientShaderInstanced() override = default;

	// Constructs the shader.
	// Returns nullptr if shader compilation failed or a valid Shader pointer on
	// success. The shader must then be deleted using default allocator Delete
	// function.
	static ozz::unique_ptr<AmbientShaderInstanced> Build();

	// Binds the shader.
	void Bind(GLsizei _models_offset, const ozz::math::Float4x4& _view_proj, GLsizei _pos_stride, GLsizei _pos_offset, GLsizei _normal_stride, GLsizei _normal_offset, GLsizei _color_stride, GLsizei _color_offset, bool _color_float);

	void Unbind() override;
};

class AmbientTexturedShader : public AmbientShader {
  public:
	// Constructs the shader.
	// Returns nullptr if shader compilation failed or a valid Shader pointer on
	// success. The shader must then be deleted using default allocator Delete
	// function.
	static ozz::unique_ptr<AmbientTexturedShader> Build();

	// Binds the shader.
	void Bind(const ozz::math::Float4x4& _model,
	          const ozz::math::Float4x4& _view_proj,
	          GLsizei                    _pos_stride,
	          GLsizei                    _pos_offset,
	          GLsizei                    _normal_stride,
	          GLsizei                    _normal_offset,
	          GLsizei                    _color_stride,
	          GLsizei                    _color_offset,
	          bool                       _color_float,
	          GLsizei                    _uv_stride,
	          GLsizei                    _uv_offset);
};
/*
class AmbientTexturedShaderInstanced : public AmbientShaderInstanced {
public:

  // Constructs the shader.
  // Returns nullptr if shader compilation failed or a valid Shader pointer on
  // success. The shader must then be deleted using default allocator Delete
  // function.
  static AmbientTexturedShaderInstanced* Build();

  // Binds the shader.
  void Bind(GLsizei _models_offset,
            const math::Float4x4& _view_proj,
            GLsizei _pos_stride, GLsizei _pos_offset,
            GLsizei _normal_stride, GLsizei _normal_offset,
            GLsizei _color_stride, GLsizei _color_offset,
            GLsizei _uv_stride, GLsizei _uv_offset);
};
*/
