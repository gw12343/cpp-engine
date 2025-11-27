//
// Created by gabe on 11/23/25.
//

#include "MaterialPreview.h"
#include "glad/glad.h"
#include "utils/Utils.h"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "core/EngineData.h"
#include "assets/AssetManager.h"
#include <vector>
#include <cmath>

namespace Engine {
	void MaterialPreview::Initialize()
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

		ENGINE_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Material preview framebuffer incomplete");

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// Generate sphere mesh
		GenerateSphere();

		initialized = true;
	}

	void MaterialPreview::GenerateSphere()
	{
		// Parametric UV sphere generation
		const int latSegments = 32;
		const int lonSegments = 32;
		const float radius = 1.0f;

		std::vector<float> vertices;
		std::vector<unsigned int> indices;

		// Generate vertices
		for (int lat = 0; lat <= latSegments; ++lat) {
			float theta = lat * M_PI / latSegments;
			float sinTheta = std::sin(theta);
			float cosTheta = std::cos(theta);

			for (int lon = 0; lon <= lonSegments; ++lon) {
				float phi = lon * 2.0f * M_PI / lonSegments;
				float sinPhi = std::sin(phi);
				float cosPhi = std::cos(phi);

				// Position
				float x = cosPhi * sinTheta;
				float y = cosTheta;
				float z = sinPhi * sinTheta;

				// Normal (same as position for unit sphere)
				float nx = x;
				float ny = y;
				float nz = z;

				// Texture coordinates
				float u = 1.0f - (float)lon / lonSegments;
				float v = 1.0f - (float)lat / latSegments;

				// Tangent (derivative along longitude)
				float tx = -sinPhi * sinTheta;
				float ty = 0.0f;
				float tz = cosPhi * sinTheta;
				float tLen = std::sqrt(tx * tx + ty * ty + tz * tz);
				if (tLen > 0.0001f) {
					tx /= tLen;
					ty /= tLen;
					tz /= tLen;
				}

				// Bitangent (derivative along latitude)
				float bx = cosPhi * cosTheta;
				float by = -sinTheta;
				float bz = sinPhi * cosTheta;
				float bLen = std::sqrt(bx * bx + by * by + bz * bz);
				if (bLen > 0.0001f) {
					bx /= bLen;
					by /= bLen;
					bz /= bLen;
				}

				// Add vertex (pos, normal, uv, tangent, bitangent)
				vertices.push_back(x * radius);
				vertices.push_back(y * radius);
				vertices.push_back(z * radius);
				vertices.push_back(nx);
				vertices.push_back(ny);
				vertices.push_back(nz);
				vertices.push_back(u);
				vertices.push_back(v);
				vertices.push_back(tx);
				vertices.push_back(ty);
				vertices.push_back(tz);
				vertices.push_back(bx);
				vertices.push_back(by);
				vertices.push_back(bz);
			}
		}

		// Generate indices
		for (int lat = 0; lat < latSegments; ++lat) {
			for (int lon = 0; lon < lonSegments; ++lon) {
				int first = (lat * (lonSegments + 1)) + lon;
				int second = first + lonSegments + 1;

				indices.push_back(first);
				indices.push_back(second);
				indices.push_back(first + 1);

				indices.push_back(second);
				indices.push_back(second + 1);
				indices.push_back(first + 1);
			}
		}

		indexCount = indices.size();

		// Create VAO, VBO, EBO
		glGenVertexArrays(1, &sphereVAO);
		glGenBuffers(1, &sphereVBO);
		glGenBuffers(1, &sphereEBO);

		glBindVertexArray(sphereVAO);

		glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

		// Position
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)0);

		// Normal
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(3 * sizeof(float)));

		// TexCoords
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(6 * sizeof(float)));

		// Tangent
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(8 * sizeof(float)));

		// Bitangent
		glEnableVertexAttribArray(4);
		glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(11 * sizeof(float)));

		glBindVertexArray(0);
	}

	void MaterialPreview::Render(Material* material, Shader& shader)
	{
		if (!initialized) {
			width = height = MATERIAL_PREVIEW_SIZE;
			Initialize();
		}

		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glViewport(0, 0, width, height);
		glEnable(GL_DEPTH_TEST);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		shader.Bind();

		// Camera setup - position to nicely view sphere
		glm::vec3 camPos = glm::vec3(2.0f, 1.5f, 2.0f);
		glm::vec3 center = glm::vec3(0.0f);

		glm::mat4 proj = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);
		glm::mat4 view = glm::lookAt(camPos, center, glm::vec3(0, 1, 0));
		glm::mat4 model = glm::mat4(1.0f);

		shader.SetMat4("projection", &proj);
		shader.SetMat4("view", &view);
		shader.SetMat4("model", &model);

		// Bind material textures
		auto& assetMgr = GetAssetManager();

		// Diffuse texture
		auto diffuseTex = assetMgr.Get(material->GetDiffuseTexture());
		if (diffuseTex) {
			diffuseTex->Bind(0);
		} else {
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, 0);
		}

		// Specular texture
		auto specularTex = assetMgr.Get(material->GetSpecularTexture());
		if (specularTex) {
			specularTex->Bind(1);
		} else {
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, 0);
		}

		// Normal texture
		auto normalTex = assetMgr.Get(material->GetNormalTexture());
		if (normalTex) {
			normalTex->Bind(2);
		} else {
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, 0);
		}

		// Set material properties
		shader.SetVec3("diffuseColor", material->GetDiffuseColor());
		shader.SetVec3("specularColor", material->GetSpecularColor());
		shader.SetFloat("shininess", material->GetShininess());

		// Draw sphere
		glBindVertexArray(sphereVAO);
		glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void MaterialPreview::Render(Material* material, Shader& shader, float yaw, float pitch)
{
	if (!initialized) {
		width = height = MATERIAL_PREVIEW_SIZE;
		Initialize();
	}

	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glViewport(0, 0, width, height);
	glEnable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	shader.Bind();

	// Camera setup using spherical coordinates
	float distance = 4.0f; // Distance from sphere center
	float yawRad = glm::radians(yaw);
	float pitchRad = glm::radians(pitch);

	// Compute camera position from spherical coordinates
	glm::vec3 camPos;
	camPos.x = distance * cos(pitchRad) * cos(yawRad);
	camPos.y = distance * sin(pitchRad);
	camPos.z = distance * cos(pitchRad) * sin(yawRad);

	glm::vec3 center = glm::vec3(0.0f);

	glm::mat4 proj = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);
	glm::mat4 view = glm::lookAt(camPos, center, glm::vec3(0, 1, 0));
	glm::mat4 model = glm::mat4(1.0f);

	shader.SetMat4("projection", &proj);
	shader.SetMat4("view", &view);
	shader.SetMat4("model", &model);

	// Bind material textures
	auto& assetMgr = GetAssetManager();

	// Diffuse texture
	auto diffuseTex = assetMgr.Get(material->GetDiffuseTexture());
	if (diffuseTex) {
		diffuseTex->Bind(0);
	} else {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	// Specular texture
	auto specularTex = assetMgr.Get(material->GetSpecularTexture());
	if (specularTex) {
		specularTex->Bind(1);
	} else {
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	// Normal texture
	auto normalTex = assetMgr.Get(material->GetNormalTexture());
	if (normalTex) {
		normalTex->Bind(2);
	} else {
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	// Set material properties
	shader.SetVec3("diffuseColor", material->GetDiffuseColor());
	shader.SetVec3("specularColor", material->GetSpecularColor());
	shader.SetFloat("shininess", material->GetShininess());

	// Draw sphere
	glBindVertexArray(sphereVAO);
	glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

} // namespace Engine
