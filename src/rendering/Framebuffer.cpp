#include "spdlog/spdlog.h"
#include "Framebuffer.h"
#include "glad/glad.h"

void Engine::Framebuffer::Resize(int width, int height)
{
	Delete(); // Clean up any previous framebuffer

	glGenFramebuffers(1, &FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);

	// Create color texture attachment
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

	// Create depth-stencil renderbuffer
	glGenRenderbuffers(1, &RBO);
	glBindRenderbuffer(GL_RENDERBUFFER, RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, RBO);

	// Check framebuffer completeness AFTER all attachments
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		spdlog::error("Framebuffer is not complete.");
	}

	Unbind();
}

void Engine::Framebuffer::Bind() const
{
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
}

void Engine::Framebuffer::Unbind()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Engine::Framebuffer::Delete()
{
	if (FBO != 0) {
		glDeleteFramebuffers(1, &FBO);
		FBO = 0;
	}
	if (texture != 0) {
		glDeleteTextures(1, &texture);
		texture = 0;
	}
	if (RBO != 0) {
		glDeleteRenderbuffers(1, &RBO);
		RBO = 0;
	}
}
