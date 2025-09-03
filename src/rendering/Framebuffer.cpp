#include "Framebuffer.h"
#include "spdlog/spdlog.h"
#include "glad/glad.h"

using namespace Engine;

void Framebuffer::Resize(int width, int height)
{
	Delete(); // cleanup

	glGenFramebuffers(1, &FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);

	if (fbType == FramebufferType::Standard) {
		// Single color attachment
		colorAttachments.resize(1);
		glGenTextures(1, &colorAttachments[0]);
		glBindTexture(GL_TEXTURE_2D, colorAttachments[0]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorAttachments[0], 0);

		// Depth-stencil RBO
		glGenRenderbuffers(1, &RBO);
		glBindRenderbuffer(GL_RENDERBUFFER, RBO);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, RBO);
	}
	else if (fbType == FramebufferType::GBuffer) {
		// Create multiple render targets: position, normal, albedo+spec
		colorAttachments.resize(3);

		// Position
		glGenTextures(1, &colorAttachments[0]);
		glBindTexture(GL_TEXTURE_2D, colorAttachments[0]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorAttachments[0], 0);

		// Normal
		glGenTextures(1, &colorAttachments[1]);
		glBindTexture(GL_TEXTURE_2D, colorAttachments[1]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, colorAttachments[1], 0);

		// Albedo + spec
		glGenTextures(1, &colorAttachments[2]);
		glBindTexture(GL_TEXTURE_2D, colorAttachments[2]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, colorAttachments[2], 0);

		// Set draw buffers
		GLuint attachments[3] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
		glDrawBuffers(3, attachments);

		// Depth buffer as renderbuffer
		glGenRenderbuffers(1, &RBO);
		glBindRenderbuffer(GL_RENDERBUFFER, RBO);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, RBO);
	}

	// Check completeness
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		spdlog::error("Framebuffer not complete.");
	}

	Unbind();
}

void Framebuffer::Bind() const
{
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
}

void Framebuffer::Unbind()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::Delete()
{
	if (FBO != 0) {
		glDeleteFramebuffers(1, &FBO);
		FBO = 0;
	}
	if (!colorAttachments.empty()) {
		glDeleteTextures((GLsizei) colorAttachments.size(), colorAttachments.data());
		colorAttachments.clear();
	}
	if (RBO != 0) {
		glDeleteRenderbuffers(1, &RBO);
		RBO = 0;
	}
}
