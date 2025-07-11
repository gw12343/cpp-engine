//----------------------------------------------------------------------------//
//                                                                            //
// ozz-animation is hosted at http://github.com/guillaumeblanc/ozz-animation  //
// and distributed under the MIT License (MIT).                               //
//                                                                            //
// Copyright (c) Guillaume Blanc                                              //
//                                                                            //
// Permission is hereby granted, free of charge, to any person obtaining a    //
// copy of this software and associated documentation files (the "Software"), //
// to deal in the Software without restriction, including without limitation  //
// the rights to use, copy, modify, merge, publish, distribute, sublicense,   //
// and/or sell copies of the Software, and to permit persons to whom the      //
// Software is furnished to do so, subject to the following conditions:       //
//                                                                            //
// The above copyright notice and this permission notice shall be included in //
// all copies or substantial portions of the Software.                        //
//                                                                            //
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR //
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   //
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    //
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER //
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    //
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        //
// DEALINGS IN THE SOFTWARE.                                                  //
//                                                                            //
//----------------------------------------------------------------------------//

#define OZZ_INCLUDE_PRIVATE_HEADER // Allows to include private headers.

#include "AnimationShader.h"

#include "ozz/base/log.h"
#include "ozz/base/maths/simd_math.h"

#include <cassert>
#include <cstdio>
#include <spdlog/spdlog.h>

#include "glad/glad.h"


static const char* kPlatformSpecificVSHeader = "#version 330\n";
static const char* kPlatformSpecificFSHeader = "#version 330\n";


void glUniformMat4(ozz::math::Float4x4 _mat4, GLint _uniform)
{
	float values[16];
	ozz::math::StorePtrU(_mat4.cols[0], values + 0);
	ozz::math::StorePtrU(_mat4.cols[1], values + 4);
	ozz::math::StorePtrU(_mat4.cols[2], values + 8);
	ozz::math::StorePtrU(_mat4.cols[3], values + 12);
	GL(UniformMatrix4fv(_uniform, 1, false, values));
}

AnimationShader::AnimationShader() : program_(0), vertex_(0), fragment_(0)
{
}

AnimationShader::~AnimationShader()
{
	if (vertex_) {
		GL(DetachShader(program_, vertex_));
		GL(DeleteShader(vertex_));
	}
	if (fragment_) {
		GL(DetachShader(program_, fragment_));
		GL(DeleteShader(fragment_));
	}
	if (program_) {
		GL(DeleteProgram(program_));
	}
}

namespace {
	GLuint CompileShader(GLenum _type, int _count, const char** _src)
	{
		GLuint shader = glCreateShader(_type);
		GL(ShaderSource(shader, _count, _src, nullptr));
		GL(CompileShader(shader));

		int infolog_length = 0;
		GL(GetShaderiv(shader, GL_INFO_LOG_LENGTH, &infolog_length));
		if (infolog_length > 1) {
			char* info_log      = reinterpret_cast<char*>(ozz::memory::default_allocator()->Allocate(infolog_length, alignof(char)));
			int   chars_written = 0;
			glGetShaderInfoLog(shader, infolog_length, &chars_written, info_log);
			spdlog::error("Shader compilation error: {}", info_log);
			ozz::memory::default_allocator()->Deallocate(info_log);
		}

		int status;
		GL(GetShaderiv(shader, GL_COMPILE_STATUS, &status));
		if (status) {
			return shader;
		}

		GL(DeleteShader(shader));
		return 0;
	}
} // namespace

bool AnimationShader::BuildFromSource(int _vertex_count, const char** _vertex, int _fragment_count, const char** _fragment)
{
	// Tries to compile shaders.
	GLuint vertex_shader = 0;
	if (_vertex) {
		vertex_shader = CompileShader(GL_VERTEX_SHADER, _vertex_count, _vertex);
		if (!vertex_shader) {
			return false;
		}
	}
	GLuint fragment_shader = 0;
	if (_fragment) {
		fragment_shader = CompileShader(GL_FRAGMENT_SHADER, _fragment_count, _fragment);
		if (!fragment_shader) {
			if (vertex_shader) {
				GL(DeleteShader(vertex_shader));
			}
			return false;
		}
	}

	// Shaders are compiled, builds program.
	program_  = glCreateProgram();
	vertex_   = vertex_shader;
	fragment_ = fragment_shader;
	GL(AttachShader(program_, vertex_shader));
	GL(AttachShader(program_, fragment_shader));
	GL(LinkProgram(program_));

	int infolog_length = 0;
	GL(GetProgramiv(program_, GL_INFO_LOG_LENGTH, &infolog_length));
	if (infolog_length > 1) {
		char* info_log      = reinterpret_cast<char*>(ozz::memory::default_allocator()->Allocate(infolog_length, alignof(char)));
		int   chars_written = 0;
		glGetProgramInfoLog(program_, infolog_length, &chars_written, info_log);
		spdlog::error("Shader compilation error: {}", info_log);
		ozz::memory::default_allocator()->Deallocate(info_log);
	}

	return true;
}

bool AnimationShader::BindUniform(const char* _semantic)
{
	if (!program_) {
		return false;
	}
	GLint location = glGetUniformLocation(program_, _semantic);
	if (glGetError() != GL_NO_ERROR || location == -1) { // _semantic not found.
		return false;
	}
	uniforms_.push_back(location);
	return true;
}

bool AnimationShader::FindAttrib(const char* _semantic)
{
	if (!program_) {
		return false;
	}
	GLint location = glGetAttribLocation(program_, _semantic);
	if (glGetError() != GL_NO_ERROR || location == -1) { // _semantic not found.
		return false;
	}
	attribs_.push_back(location);
	return true;
}

void AnimationShader::UnbindAttribs()
{
	for (int attrib : attribs_) {
		GL(DisableVertexAttribArray(attrib));
	}
}

void AnimationShader::Unbind()
{
	UnbindAttribs();
	GL(UseProgram(0));
}

ozz::unique_ptr<ImmediatePCShader> ImmediatePCShader::Build()
{
	bool success = true;

	const char* kSimplePCVS = "uniform mat4 u_mvp;\n"
	                          "in vec3 a_position;\n"
	                          "in vec4 a_color;\n"
	                          "out vec4 v_vertex_color;\n"
	                          "void main() {\n"
	                          "  vec4 vertex = vec4(a_position.xyz, 1.);\n"
	                          "  gl_Position = u_mvp * vertex;\n"
	                          "  v_vertex_color = a_color;\n"
	                          "}\n";
	const char* kSimplePCPS = "in vec4 v_vertex_color;\n"
	                          "out vec4 o_color;\n"
	                          "void main() {\n"
	                          "  o_color = v_vertex_color;\n"
	                          "}\n";

	const char* vs[] = {kPlatformSpecificVSHeader, kSimplePCVS};
	const char* fs[] = {kPlatformSpecificFSHeader, kSimplePCPS};

	ozz::unique_ptr<ImmediatePCShader> shader = ozz::make_unique<ImmediatePCShader>();
	success &= shader->BuildFromSource(OZZ_ARRAY_SIZE(vs), vs, OZZ_ARRAY_SIZE(fs), fs);

	// Binds default attributes
	success &= shader->FindAttrib("a_position");
	success &= shader->FindAttrib("a_color");

	// Binds default uniforms
	success &= shader->BindUniform("u_mvp");

	if (!success) {
		shader.reset();
	}

	return shader;
}

void ImmediatePCShader::Bind(const ozz::math::Float4x4& _model, const ozz::math::Float4x4& _view_proj, GLsizei _pos_stride, GLsizei _pos_offset, GLsizei _color_stride, GLsizei _color_offset)
{
	GL(UseProgram(program()));

	const GLint position_attrib = attrib(0);
	GL(EnableVertexAttribArray(position_attrib));
	GL(VertexAttribPointer(position_attrib, 3, GL_FLOAT, GL_FALSE, _pos_stride, GL_PTR_OFFSET(_pos_offset)));

	const GLint color_attrib = attrib(1);
	GL(EnableVertexAttribArray(color_attrib));
	GL(VertexAttribPointer(color_attrib, 4, GL_FLOAT, GL_FALSE, _color_stride, GL_PTR_OFFSET(_color_offset)));

	// Binds mvp uniform
	glUniformMat4(_view_proj * _model, uniform(0));
}

ozz::unique_ptr<ImmediatePTCShader> ImmediatePTCShader::Build()
{
	bool success = true;

	const char* kSimplePCVS = "uniform mat4 u_mvp;\n"
	                          "in vec3 a_position;\n"
	                          "in vec2 a_tex_coord;\n"
	                          "in vec4 a_color;\n"
	                          "out vec4 v_vertex_color;\n"
	                          "out vec2 v_texture_coord;\n"
	                          "void main() {\n"
	                          "  vec4 vertex = vec4(a_position.xyz, 1.);\n"
	                          "  gl_Position = u_mvp * vertex;\n"
	                          "  v_vertex_color = a_color;\n"
	                          "  v_texture_coord = a_tex_coord;\n"
	                          "}\n";
	const char* kSimplePCPS = "uniform sampler2D u_texture;\n"
	                          "in vec4 v_vertex_color;\n"
	                          "in vec2 v_texture_coord;\n"
	                          "out vec4 o_color;\n"
	                          "void main() {\n"
	                          "  vec4 tex_color = texture(u_texture, v_texture_coord);\n"
	                          "  o_color = v_vertex_color * tex_color;\n"
	                          "  if(o_color.a < .01) discard;\n" // Implements alpha testing.
	                          "}\n";

	const char* vs[] = {kPlatformSpecificVSHeader, kSimplePCVS};
	const char* fs[] = {kPlatformSpecificFSHeader, kSimplePCPS};

	ozz::unique_ptr<ImmediatePTCShader> shader = ozz::make_unique<ImmediatePTCShader>();
	success &= shader->BuildFromSource(OZZ_ARRAY_SIZE(vs), vs, OZZ_ARRAY_SIZE(fs), fs);

	// Binds default attributes
	success &= shader->FindAttrib("a_position");
	success &= shader->FindAttrib("a_tex_coord");
	success &= shader->FindAttrib("a_color");

	// Binds default uniforms
	success &= shader->BindUniform("u_mvp");
	success &= shader->BindUniform("u_texture");

	if (!success) {
		shader.reset();
	}

	return shader;
}

void ImmediatePTCShader::Bind(const ozz::math::Float4x4& _model, const ozz::math::Float4x4& _view_proj, GLsizei _pos_stride, GLsizei _pos_offset, GLsizei _tex_stride, GLsizei _tex_offset, GLsizei _color_stride, GLsizei _color_offset)
{
	GL(UseProgram(program()));

	const GLint position_attrib = attrib(0);
	GL(EnableVertexAttribArray(position_attrib));
	GL(VertexAttribPointer(position_attrib, 3, GL_FLOAT, GL_FALSE, _pos_stride, GL_PTR_OFFSET(_pos_offset)));

	const GLint tex_attrib = attrib(1);
	GL(EnableVertexAttribArray(tex_attrib));
	GL(VertexAttribPointer(tex_attrib, 2, GL_FLOAT, GL_FALSE, _tex_stride, GL_PTR_OFFSET(_tex_offset)));

	const GLint color_attrib = attrib(2);
	GL(EnableVertexAttribArray(color_attrib));
	GL(VertexAttribPointer(color_attrib, 4, GL_FLOAT, GL_FALSE, _color_stride, GL_PTR_OFFSET(_color_offset)));

	// Binds mvp uniform
	glUniformMat4(_view_proj * _model, uniform(0));

	// Binds texture
	const GLint texture = uniform(1);
	GL(Uniform1i(texture, 0));
}

ozz::unique_ptr<PointsShader> PointsShader::Build()
{
	bool success = true;

	const char* kSimplePointsVS = "uniform mat4 u_mvp;\n"
	                              "in vec3 a_position;\n"
	                              "in vec4 a_color;\n"
	                              "in float a_size;\n"
	                              "in float a_screen_space;\n"
	                              "out vec4 v_vertex_color;\n"
	                              "void main() {\n"
	                              "  vec4 vertex = vec4(a_position.xyz, 1.);\n"
	                              "  gl_Position = u_mvp * vertex;\n"
	                              "  gl_PointSize = a_screen_space == 0. ? a_size / gl_Position.w : "
	                              "a_size;\n"
	                              "  v_vertex_color = a_color;\n"
	                              "}\n";
	const char* kSimplePointsPS = "in vec4 v_vertex_color;\n"
	                              "out vec4 o_color;\n"
	                              "void main() {\n"
	                              "  o_color = v_vertex_color;\n"
	                              "}\n";

	const char* vs[] = {kPlatformSpecificVSHeader, kSimplePointsVS};
	const char* fs[] = {kPlatformSpecificFSHeader, kSimplePointsPS};

	ozz::unique_ptr<PointsShader> shader = ozz::make_unique<PointsShader>();
	success &= shader->BuildFromSource(OZZ_ARRAY_SIZE(vs), vs, OZZ_ARRAY_SIZE(fs), fs);

	// Binds default attributes
	success &= shader->FindAttrib("a_position");
	success &= shader->FindAttrib("a_color");
	success &= shader->FindAttrib("a_size");
	success &= shader->FindAttrib("a_screen_space");

	// Binds default uniforms
	success &= shader->BindUniform("u_mvp");

	if (!success) {
		shader.reset();
	}

	return shader;
}

PointsShader::GenericAttrib PointsShader::Bind(
    const ozz::math::Float4x4& _model, const ozz::math::Float4x4& _view_proj, GLsizei _pos_stride, GLsizei _pos_offset, GLsizei _color_stride, GLsizei _color_offset, GLsizei _size_stride, GLsizei _size_offset, bool _screen_space)
{
	GL(UseProgram(program()));

	const GLint position_attrib = attrib(0);
	GL(EnableVertexAttribArray(position_attrib));
	GL(VertexAttribPointer(position_attrib, 3, GL_FLOAT, GL_FALSE, _pos_stride, GL_PTR_OFFSET(_pos_offset)));

	const GLint color_attrib = attrib(1);
	if (_color_stride) {
		GL(EnableVertexAttribArray(color_attrib));
		GL(VertexAttribPointer(color_attrib, 4, GL_FLOAT, GL_FALSE, _color_stride, GL_PTR_OFFSET(_color_offset)));
	}
	const GLint size_attrib = attrib(2);
	if (_size_stride) {
		GL(EnableVertexAttribArray(size_attrib));
		GL(VertexAttribPointer(size_attrib, 1, GL_FLOAT, GL_TRUE, _size_stride, GL_PTR_OFFSET(_size_offset)));
	}
	const GLint screen_space_attrib = attrib(3);
	GL(VertexAttrib1f(screen_space_attrib, 1.f * _screen_space));

	// Binds mvp uniform
	glUniformMat4(_view_proj * _model, uniform(0));

	return {_color_stride ? -1 : color_attrib, _size_stride ? -1 : size_attrib};
}

namespace {
	const char* kPassUv                  = "in vec2 a_uv;\n"
	                                       "out vec2 v_vertex_uv;\n"
	                                       "void PassUv() {\n"
	                                       "  v_vertex_uv = a_uv;\n"
	                                       "}\n";
	const char* kPassNoUv                = "void PassUv() {\n"
	                                       "}\n";
	const char* kShaderUberVS            = "uniform mat4 u_viewproj;\n"
	                                       "in vec3 a_position;\n"
	                                       "in vec3 a_normal;\n"
	                                       "in vec4 a_color;\n"
	                                       "out vec3 v_world_normal;\n"
	                                       "out vec4 v_vertex_color;\n"
	                                       "void main() {\n"
	                                       "  mat4 world_matrix = GetWorldMatrix();\n"
	                                       "  vec4 vertex = vec4(a_position.xyz, 1.);\n"
	                                       "  gl_Position = u_viewproj * world_matrix * vertex;\n"
	                                       "  mat3 cross_matrix = mat3(\n"
	                                       "    cross(world_matrix[1].xyz, world_matrix[2].xyz),\n"
	                                       "    cross(world_matrix[2].xyz, world_matrix[0].xyz),\n"
	                                       "    cross(world_matrix[0].xyz, world_matrix[1].xyz));\n"
	                                       "  float invdet = 1.0 / dot(cross_matrix[2], world_matrix[2].xyz);\n"
	                                       "  mat3 normal_matrix = cross_matrix * invdet;\n"
	                                       "  v_world_normal = normal_matrix * a_normal;\n"
	                                       "  v_vertex_color = a_color;\n"
	                                       "  PassUv();\n"
	                                       "}\n";
	const char* kShaderAmbientFct        = "vec4 GetAmbient(vec3 _world_normal) {\n"
	                                       "  vec3 normal = normalize(_world_normal);\n"
	                                       "  vec3 alpha = (normal + 1.) * .5;\n"
	                                       "  vec2 bt = mix(vec2(.3, .7), vec2(.4, .8), alpha.xz);\n"
	                                       "  vec3 ambient = mix(vec3(bt.x, .3, bt.x), vec3(bt.y, .8, bt.y), "
	                                       "alpha.y);\n"
	                                       "  return vec4(ambient, 1.);\n"
	                                       "}\n";
	const char* kShaderAmbientFS         = "in vec3 v_world_normal;\n"
	                                       "in vec4 v_vertex_color;\n"
	                                       "out vec4 o_color;\n"
	                                       "void main() {\n"
	                                       "  vec4 ambient = GetAmbient(v_world_normal);\n"
	                                       "  o_color = ambient *\n"
	                                       "                 v_vertex_color;\n"
	                                       "}\n";
	const char* kShaderAmbientTexturedFS = "uniform sampler2D u_texture;\n"
	                                       "in vec3 v_world_normal;\n"
	                                       "in vec4 v_vertex_color;\n"
	                                       "in vec2 v_vertex_uv;\n"
	                                       "out vec4 o_color;\n"
	                                       "void main() {\n"
	                                       "  vec4 ambient = GetAmbient(v_world_normal);\n"
	                                       "  o_color = ambient *\n"
	                                       "                 v_vertex_color *\n"
	                                       "                 texture(u_texture, v_vertex_uv);\n"
	                                       "}\n";
} // namespace

void SkeletonShader::Bind(const ozz::math::Float4x4& _model, const ozz::math::Float4x4& _view_proj, GLsizei _pos_stride, GLsizei _pos_offset, GLsizei _normal_stride, GLsizei _normal_offset, GLsizei _color_stride, GLsizei _color_offset)
{
	GL(UseProgram(program()));

	const GLint position_attrib = attrib(0);
	GL(EnableVertexAttribArray(position_attrib));
	GL(VertexAttribPointer(position_attrib, 3, GL_FLOAT, GL_FALSE, _pos_stride, GL_PTR_OFFSET(_pos_offset)));

	const GLint normal_attrib = attrib(1);
	GL(EnableVertexAttribArray(normal_attrib));
	GL(VertexAttribPointer(normal_attrib, 3, GL_FLOAT, GL_FALSE, _normal_stride, GL_PTR_OFFSET(_normal_offset)));

	const GLint color_attrib = attrib(2);
	GL(EnableVertexAttribArray(color_attrib));
	GL(VertexAttribPointer(color_attrib, 4, GL_FLOAT, GL_FALSE, _color_stride, GL_PTR_OFFSET(_color_offset)));

	// Binds vp uniform
	glUniformMat4(_model, uniform(0));

	// Binds vp uniform
	glUniformMat4(_view_proj, uniform(1));
}

ozz::unique_ptr<JointShader> JointShader::Build()
{
	bool success = true;

	const char* vs_joint_to_world_matrix = "uniform mat4 u_model;\n"
	                                       "mat4 GetWorldMatrix() {\n"
	                                       "  // Rebuilds joint matrix.\n"
	                                       "  mat4 joint_matrix;\n"
	                                       "  joint_matrix[0] = vec4(normalize(joint[0].xyz), 0.);\n"
	                                       "  joint_matrix[1] = vec4(normalize(joint[1].xyz), 0.);\n"
	                                       "  joint_matrix[2] = vec4(normalize(joint[2].xyz), 0.);\n"
	                                       "  joint_matrix[3] = vec4(joint[3].xyz, 1.);\n"

	                                       "  // Rebuilds bone properties.\n"
	                                       "  vec3 bone_dir = vec3(joint[0].w, joint[1].w, joint[2].w);\n"
	                                       "  float bone_len = length(bone_dir);\n"

	                                       "  // Setup rendering world matrix.\n"
	                                       "  mat4 world_matrix;\n"
	                                       "  world_matrix[0] = joint_matrix[0] * bone_len;\n"
	                                       "  world_matrix[1] = joint_matrix[1] * bone_len;\n"
	                                       "  world_matrix[2] = joint_matrix[2] * bone_len;\n"
	                                       "  world_matrix[3] = joint_matrix[3];\n"
	                                       "  return u_model * world_matrix;\n"
	                                       "}\n";

	const char* vs[] = {kPlatformSpecificVSHeader, kPassNoUv, "uniform mat4 joint;\n", vs_joint_to_world_matrix, kShaderUberVS};
	const char* fs[] = {kPlatformSpecificFSHeader, kShaderAmbientFct, kShaderAmbientFS};

	ozz::unique_ptr<JointShader> shader = ozz::make_unique<JointShader>();
	success &= shader->BuildFromSource(OZZ_ARRAY_SIZE(vs), vs, OZZ_ARRAY_SIZE(fs), fs);

	// Binds default attributes
	success &= shader->FindAttrib("a_position");
	success &= shader->FindAttrib("a_normal");
	success &= shader->FindAttrib("a_color");

	// Binds default uniforms
	success &= shader->BindUniform("u_model");
	success &= shader->BindUniform("u_viewproj");

	success &= shader->BindUniform("joint");

	if (!success) {
		shader.reset();
	}

	return shader;
}

ozz::unique_ptr<BoneShader> BoneShader::Build()
{ // Builds a world matrix from joint uniforms,
  // sticking bone model between
	bool success = true;

	// parent and child joints.
	const char* vs_joint_to_world_matrix = "uniform mat4 u_model;\n"
	                                       "mat4 GetWorldMatrix() {\n"
	                                       "  // Rebuilds bone properties.\n"
	                                       "  // Bone length is set to zero to disable leaf rendering.\n"
	                                       "  float is_bone = joint[3].w;\n"
	                                       "  vec3 bone_dir = vec3(joint[0].w, joint[1].w, joint[2].w) * is_bone;\n"
	                                       "  float bone_len = length(bone_dir);\n"

	                                       "  // Setup rendering world matrix.\n"
	                                       "  float dot1 = dot(joint[2].xyz, bone_dir);\n"
	                                       "  float dot2 = dot(joint[0].xyz, bone_dir);\n"
	                                       "  vec3 binormal = abs(dot1) < abs(dot2) ? joint[2].xyz : joint[0].xyz;\n"

	                                       "  mat4 world_matrix;\n"
	                                       "  world_matrix[0] = vec4(bone_dir, 0.);\n"
	                                       "  world_matrix[1] = \n"
	                                       "    vec4(bone_len * normalize(cross(binormal, bone_dir)), 0.);\n"
	                                       "  world_matrix[2] =\n"
	                                       "    vec4(bone_len * normalize(cross(bone_dir, world_matrix[1].xyz)), "
	                                       "0.);\n"
	                                       "  world_matrix[3] = vec4(joint[3].xyz, 1.);\n"
	                                       "  return u_model * world_matrix;\n"
	                                       "}\n";
	const char* vs[] = {kPlatformSpecificVSHeader, kPassNoUv, "uniform mat4 joint;\n", vs_joint_to_world_matrix, kShaderUberVS};
	const char* fs[] = {kPlatformSpecificFSHeader, kShaderAmbientFct, kShaderAmbientFS};

	ozz::unique_ptr<BoneShader> shader = ozz::make_unique<BoneShader>();
	success &= shader->BuildFromSource(OZZ_ARRAY_SIZE(vs), vs, OZZ_ARRAY_SIZE(fs), fs);

	// Binds default attributes
	success &= shader->FindAttrib("a_position");
	success &= shader->FindAttrib("a_normal");
	success &= shader->FindAttrib("a_color");

	// Binds default uniforms
	success &= shader->BindUniform("u_model");
	success &= shader->BindUniform("u_viewproj");

	success &= shader->BindUniform("joint");

	if (!success) {
		spdlog::error("Failed to build bone shader");
		shader.reset();
	}

	return shader;
}

ozz::unique_ptr<AmbientShader> AmbientShader::Build()
{
	const char* vs[] = {kPlatformSpecificVSHeader,
	                    kPassNoUv,
	                    "uniform mat4 u_model;\n"
	                    "mat4 GetWorldMatrix() {return u_model;}\n",
	                    kShaderUberVS};
	const char* fs[] = {kPlatformSpecificFSHeader, kShaderAmbientFct, kShaderAmbientFS};

	ozz::unique_ptr<AmbientShader> shader  = ozz::make_unique<AmbientShader>();
	bool                           success = shader->InternalBuild(OZZ_ARRAY_SIZE(vs), vs, OZZ_ARRAY_SIZE(fs), fs);

	if (!success) {
		shader.reset();
	}

	return shader;
}

bool AmbientShader::InternalBuild(int _vertex_count, const char** _vertex, int _fragment_count, const char** _fragment)
{
	bool success = true;

	success &= BuildFromSource(_vertex_count, _vertex, _fragment_count, _fragment);

	// Binds default attributes
	success &= FindAttrib("a_position");
	success &= FindAttrib("a_normal");
	success &= FindAttrib("a_color");

	// Binds default uniforms
	success &= BindUniform("u_model");
	success &= BindUniform("u_viewproj");

	return success;
}

void AmbientShader::Bind(
    const ozz::math::Float4x4& _model, const ozz::math::Float4x4& _view_proj, GLsizei _pos_stride, GLsizei _pos_offset, GLsizei _normal_stride, GLsizei _normal_offset, GLsizei _color_stride, GLsizei _color_offset, bool _color_float)
{
	GL(UseProgram(program()));

	const GLint position_attrib = attrib(0);
	GL(EnableVertexAttribArray(position_attrib));
	GL(VertexAttribPointer(position_attrib, 3, GL_FLOAT, GL_FALSE, _pos_stride, GL_PTR_OFFSET(_pos_offset)));

	const GLint normal_attrib = attrib(1);
	GL(EnableVertexAttribArray(normal_attrib));
	GL(VertexAttribPointer(normal_attrib, 3, GL_FLOAT, GL_FALSE, _normal_stride, GL_PTR_OFFSET(_normal_offset)));

	const GLint color_attrib = attrib(2);
	GL(EnableVertexAttribArray(color_attrib));
	GL(VertexAttribPointer(color_attrib, 4, _color_float ? GL_FLOAT : GL_UNSIGNED_BYTE, !_color_float, _color_stride, GL_PTR_OFFSET(_color_offset)));

	// Binds mw uniform
	glUniformMat4(_model, uniform(0));

	// Binds mvp uniform
	glUniformMat4(_view_proj, uniform(1));
}

ozz::unique_ptr<AmbientShaderInstanced> AmbientShaderInstanced::Build()
{
	bool success = true;

	const char* vs[] = {kPlatformSpecificVSHeader, kPassNoUv, "in mat4 a_m;\n mat4 GetWorldMatrix() {return a_m;}\n", kShaderUberVS};
	const char* fs[] = {kPlatformSpecificFSHeader, kShaderAmbientFct, kShaderAmbientFS};

	ozz::unique_ptr<AmbientShaderInstanced> shader = ozz::make_unique<AmbientShaderInstanced>();
	success &= shader->BuildFromSource(OZZ_ARRAY_SIZE(vs), vs, OZZ_ARRAY_SIZE(fs), fs);

	// Binds default attributes
	success &= shader->FindAttrib("a_position");
	success &= shader->FindAttrib("a_normal");
	success &= shader->FindAttrib("a_color");
	success &= shader->FindAttrib("a_m");

	// Binds default uniforms
	success &= shader->BindUniform("u_viewproj");

	if (!success) {
		shader.reset();
	}

	return shader;
}

void AmbientShaderInstanced::Bind(
    GLsizei _models_offset, const ozz::math::Float4x4& _view_proj, GLsizei _pos_stride, GLsizei _pos_offset, GLsizei _normal_stride, GLsizei _normal_offset, GLsizei _color_stride, GLsizei _color_offset, bool _color_float)
{
	GL(UseProgram(program()));

	const GLint position_attrib = attrib(0);
	GL(EnableVertexAttribArray(position_attrib));
	GL(VertexAttribPointer(position_attrib, 3, GL_FLOAT, GL_FALSE, _pos_stride, GL_PTR_OFFSET(_pos_offset)));

	const GLint normal_attrib = attrib(1);
	GL(EnableVertexAttribArray(normal_attrib));
	GL(VertexAttribPointer(normal_attrib, 3, GL_FLOAT, GL_FALSE, _normal_stride, GL_PTR_OFFSET(_normal_offset)));

	const GLint color_attrib = attrib(2);
	GL(EnableVertexAttribArray(color_attrib));
	GL(VertexAttribPointer(color_attrib, 4, _color_float ? GL_FLOAT : GL_UNSIGNED_BYTE, !_color_float, _color_stride, GL_PTR_OFFSET(_color_offset)));
	if (_color_stride == 0) {
		GL(VertexAttribDivisor(color_attrib, 0xffffffff));
	}

	// Binds mw uniform
	const GLint models_attrib = attrib(3);
	GL(EnableVertexAttribArray(models_attrib + 0));
	GL(EnableVertexAttribArray(models_attrib + 1));
	GL(EnableVertexAttribArray(models_attrib + 2));
	GL(EnableVertexAttribArray(models_attrib + 3));
	GL(VertexAttribDivisor(models_attrib + 0, 1));
	GL(VertexAttribDivisor(models_attrib + 1, 1));
	GL(VertexAttribDivisor(models_attrib + 2, 1));
	GL(VertexAttribDivisor(models_attrib + 3, 1));
	GL(VertexAttribPointer(models_attrib + 0, 4, GL_FLOAT, GL_FALSE, sizeof(ozz::math::Float4x4), GL_PTR_OFFSET(0 + _models_offset)));
	GL(VertexAttribPointer(models_attrib + 1, 4, GL_FLOAT, GL_FALSE, sizeof(ozz::math::Float4x4), GL_PTR_OFFSET(16 + _models_offset)));
	GL(VertexAttribPointer(models_attrib + 2, 4, GL_FLOAT, GL_FALSE, sizeof(ozz::math::Float4x4), GL_PTR_OFFSET(32 + _models_offset)));
	GL(VertexAttribPointer(models_attrib + 3, 4, GL_FLOAT, GL_FALSE, sizeof(ozz::math::Float4x4), GL_PTR_OFFSET(48 + _models_offset)));

	// Binds mvp uniform
	glUniformMat4(_view_proj, uniform(0));
}

void AmbientShaderInstanced::Unbind()
{
	const GLint color_attrib = attrib(2);
	GL(VertexAttribDivisor(color_attrib, 0));

	const GLint models_attrib = attrib(3);
	GL(DisableVertexAttribArray(models_attrib + 0));
	GL(DisableVertexAttribArray(models_attrib + 1));
	GL(DisableVertexAttribArray(models_attrib + 2));
	GL(DisableVertexAttribArray(models_attrib + 3));
	GL(VertexAttribDivisor(models_attrib + 0, 0));
	GL(VertexAttribDivisor(models_attrib + 1, 0));
	GL(VertexAttribDivisor(models_attrib + 2, 0));
	GL(VertexAttribDivisor(models_attrib + 3, 0));
	AnimationShader::Unbind();
}

ozz::unique_ptr<AmbientTexturedShader> AmbientTexturedShader::Build()
{
	const char* vs[] = {kPlatformSpecificVSHeader, kPassUv, "uniform mat4 u_model;\n mat4 GetWorldMatrix() {return u_model;}\n", kShaderUberVS};
	const char* fs[] = {kPlatformSpecificFSHeader, kShaderAmbientFct, kShaderAmbientTexturedFS};

	ozz::unique_ptr<AmbientTexturedShader> shader  = ozz::make_unique<AmbientTexturedShader>();
	bool                                   success = shader->InternalBuild(OZZ_ARRAY_SIZE(vs), vs, OZZ_ARRAY_SIZE(fs), fs);

	success &= shader->FindAttrib("a_uv");

	if (!success) {
		shader.reset();
	}

	return shader;
}

void AmbientTexturedShader::Bind(const ozz::math::Float4x4& _model,
                                 const ozz::math::Float4x4& _view_proj,
                                 GLsizei                    _pos_stride,
                                 GLsizei                    _pos_offset,
                                 GLsizei                    _normal_stride,
                                 GLsizei                    _normal_offset,
                                 GLsizei                    _color_stride,
                                 GLsizei                    _color_offset,
                                 bool                       _color_float,
                                 GLsizei                    _uv_stride,
                                 GLsizei                    _uv_offset)
{
	AmbientShader::Bind(_model, _view_proj, _pos_stride, _pos_offset, _normal_stride, _normal_offset, _color_stride, _color_offset, _color_float);

	const GLint uv_attrib = attrib(3);
	GL(EnableVertexAttribArray(uv_attrib));
	GL(VertexAttribPointer(uv_attrib, 2, GL_FLOAT, GL_FALSE, _uv_stride, GL_PTR_OFFSET(_uv_offset)));
}
