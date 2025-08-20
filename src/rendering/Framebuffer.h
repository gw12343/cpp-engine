#pragma once


typedef unsigned int GLuint;
typedef int          GLint;

#define GL_NEAREST 0x2600
#define GL_LINEAR  0x2601

namespace Engine {
	class Framebuffer {
	  public:
		explicit Framebuffer(GLint minfilt = GL_LINEAR, GLint maxfilt = GL_LINEAR) : min_filter(minfilt), mag_filter(maxfilt) {}

		void        Bind() const;
		static void Unbind();
		void        Resize(int width, int height);
		void        Delete();
		GLuint      texture = 0;
		GLint       min_filter;
		GLint       mag_filter;

	  private:
		GLuint FBO = 0;
		GLuint RBO = 0;
	};
} // namespace Engine