#pragma once


#include "glad/glad.h"
namespace Engine {
	class Framebuffer {
	  public:
		void        Bind() const;
		static void Unbind();
		void        Resize(int width, int height);
		void        Delete();
		GLuint      texture = 0;

	  private:
		GLuint FBO = 0;
		GLuint RBO = 0;
	};
} // namespace Engine