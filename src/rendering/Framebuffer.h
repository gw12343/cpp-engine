#pragma once

#include <vector>
#include <cstddef>
typedef unsigned int GLuint;
typedef int          GLint;

#define GL_NEAREST 0x2600
#define GL_LINEAR  0x2601

namespace Engine {

	enum class FramebufferType {
		Standard, // 1 color + depth/stencil
		GBuffer   // multiple MRTs + depth
	};

	class Framebuffer {
	  public:
		explicit Framebuffer(FramebufferType type = FramebufferType::Standard, GLint minfilt = GL_LINEAR, GLint magfilt = GL_LINEAR) : fbType(type), min_filter(minfilt), mag_filter(magfilt) {}

		void        Bind() const;
		static void Unbind();
		void        Resize(int width, int height);
		void        Delete();

		GLuint GetColorAttachment(size_t index = 0) const { return (index < colorAttachments.size()) ? colorAttachments[index] : 0; }

		GLuint GetDepthAttachment() const { return depthAttachment; }


		GLuint GetFBO() const { return FBO; }

	  private:
		FramebufferType     fbType;
		GLuint              FBO = 0;
		GLuint              RBO = 0; // used only for Standard framebuffer depth/stencil
		std::vector<GLuint> colorAttachments;
		GLuint              depthAttachment = 0;

		GLint min_filter;
		GLint mag_filter;
	};

} // namespace Engine