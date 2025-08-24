//
// Created by gabe on 7/1/25.
//

#include "ModelPreview.h"
#include "glad/glad.h"
#include "utils/Utils.h"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"

namespace Engine {
	void ModelPreview::Initialize()
	{
		if (fbo) glDeleteFramebuffers(1, &fbo);
		if (texture) glDeleteTextures(1, &texture);
		if (depth) glDeleteRenderbuffers(1, &depth);

		glGenFramebuffers(1, &fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);

		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

		glGenRenderbuffers(1, &depth);
		glBindRenderbuffer(GL_RENDERBUFFER, depth);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth);

		ENGINE_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Preview framebuffer incomplete");

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		initialized = true;
	}


	void ModelPreview::Render(Rendering::Model* model, Shader& shader)
	{
		if (!initialized) {
			width = height = MODEL_PREVIEW_SIZE;
			Initialize();
		}

		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glViewport(0, 0, width, height);
		glEnable(GL_DEPTH_TEST);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		shader.Bind();

		// Compute bounding box center and radius
		glm::vec3 center  = (model->m_boundsMin + model->m_boundsMax) * 0.5f;
		glm::vec3 extents = model->m_boundsMax - model->m_boundsMin;
		float     radius  = glm::length(extents) * 0.5f;

		// Camera setup: back up along diagonal at 2.5x radius
		glm::vec3 camDir = glm::normalize(glm::vec3(1.0f, 1.0f, 1.0f));
		glm::vec3 camPos = center + camDir * (radius * 2.5f);

		glm::mat4 proj     = glm::perspective(glm::radians(45.0f), 1.f, 0.1f, radius * 6.0f);
		glm::mat4 view     = glm::lookAt(camPos, center, glm::vec3(0, 1, 0));
		glm::mat4 modelMat = glm::mat4(1.0f);

		shader.SetMat4("projection", &proj);
		shader.SetMat4("view", &view);
		shader.SetMat4("model", &modelMat);

		model->Draw(shader, true);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

} // namespace Engine