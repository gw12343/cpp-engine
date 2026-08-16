//
// Created by gabe on 7/1/25.
//

#include "ModelPreview.h"


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


	static void EncapsulateTransformedAabb(glm::vec3& outMin, glm::vec3& outMax, bool& any, const glm::mat4& world, const glm::vec3& bmin, const glm::vec3& bmax)
	{
		const glm::vec3 corners[8] = {
		    {bmin.x, bmin.y, bmin.z},
		    {bmax.x, bmin.y, bmin.z},
		    {bmin.x, bmax.y, bmin.z},
		    {bmax.x, bmax.y, bmin.z},
		    {bmin.x, bmin.y, bmax.z},
		    {bmax.x, bmin.y, bmax.z},
		    {bmin.x, bmax.y, bmax.z},
		    {bmax.x, bmax.y, bmax.z},
		};
		for (const glm::vec3& c : corners) {
			const glm::vec3 w = glm::vec3(world * glm::vec4(c, 1.0f));
			if (!any) {
				outMin = outMax = w;
				any             = true;
			}
			else {
				outMin = glm::min(outMin, w);
				outMax = glm::max(outMax, w);
			}
		}
	}

	void ModelPreview::Render(Rendering::Model* model, Shader& shader)
	{
		if (!model) return;
		PreviewDrawItem item;
		item.model = model;
		item.world = glm::mat4(1.0f);
		Render(std::vector<PreviewDrawItem>{item}, shader);
	}

	void ModelPreview::Render(const std::vector<PreviewDrawItem>& items, Shader& shader)
	{
		if (!initialized) {
			width = height = MODEL_PREVIEW_SIZE;
			Initialize();
		}

		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glViewport(0, 0, width, height);
		glEnable(GL_DEPTH_TEST);
		GLfloat prevClear[4];
		glGetFloatv(GL_COLOR_CLEAR_VALUE, prevClear);
		glClearColor(0.12f, 0.12f, 0.14f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glm::vec3 bmin(0.0f);
		glm::vec3 bmax(0.0f);
		bool      any = false;
		for (const auto& item : items) {
			if (!item.model) continue;
			EncapsulateTransformedAabb(bmin, bmax, any, item.world, item.model->m_boundsMin, item.model->m_boundsMax);
		}
		if (!any) {
			glClearColor(prevClear[0], prevClear[1], prevClear[2], prevClear[3]);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			return;
		}

		shader.Bind();

		const glm::vec3 center  = (bmin + bmax) * 0.5f;
		const glm::vec3 extents = bmax - bmin;
		float           radius  = glm::length(extents) * 0.5f;
		if (radius < 1e-4f) {
			radius = 0.5f;
		}

		const glm::vec3 camDir   = glm::normalize(glm::vec3(1.0f, 0.85f, 1.0f));
		const float     fov      = 40.0f;
		float           distance = radius / glm::tan(glm::radians(fov * 0.5f));
		distance *= 1.35f;
		const glm::vec3 camPos = center + camDir * distance;

		const float   nearPlane = glm::max(0.01f, distance - radius * 2.5f);
		const float   farPlane  = distance + radius * 2.5f;
		glm::mat4     proj      = glm::perspective(glm::radians(fov), 1.f, nearPlane, farPlane);
		glm::mat4     view      = glm::lookAt(camPos, center, glm::vec3(0, 1, 0));

		shader.SetMat4("projection", &proj);
		shader.SetMat4("view", &view);

		for (const auto& item : items) {
			if (!item.model) continue;
			glm::mat4 modelMat = item.world;
			shader.SetMat4("model", &modelMat);
			if (item.materialOverrides.empty()) {
				item.model->Draw(shader, false, true);
			}
			else {
				item.model->Draw(shader, false, true, item.materialOverrides);
			}
		}

		glClearColor(prevClear[0], prevClear[1], prevClear[2], prevClear[3]);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

} // namespace Engine