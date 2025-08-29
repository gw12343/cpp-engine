#include "Mesh.h"

#include "Shader.h"
#include "core/Engine.h"
#include "terrain/TerrainManager.h"
#include "utils/Utils.h"
#include "core/EngineData.h"
#include "assets/AssetManager.h"

#include <glad/glad.h>
#include <spdlog/spdlog.h>
#include "rendering/Renderer.h"

namespace Engine {
	namespace Rendering {
		// Static tracking sets
		std::unordered_set<GLuint> Mesh::s_vaos;
		std::unordered_set<GLuint> Mesh::s_vbos;
		std::unordered_set<GLuint> Mesh::s_ebos;

		Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices, const std::shared_ptr<Material>& material) : m_vertices(vertices), m_indices(indices), m_material(material), m_vao(0), m_vbo(0), m_ebo(0)
		{
			SetupMesh();
		}

		void Mesh::SetupMesh()
		{
			glGenVertexArrays(1, &m_vao);
			glGenBuffers(1, &m_vbo);
			glGenBuffers(1, &m_ebo);

			s_vaos.insert(m_vao);
			s_vbos.insert(m_vbo);
			s_ebos.insert(m_ebo);

			glBindVertexArray(m_vao);

			glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
			glBufferData(GL_ARRAY_BUFFER, static_cast<int>(m_vertices.size() * sizeof(Vertex)), &m_vertices[0], GL_STATIC_DRAW);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<int>(m_indices.size() * sizeof(unsigned int)), &m_indices[0], GL_STATIC_DRAW);

			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) nullptr);

			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, Normal));

			glEnableVertexAttribArray(2);
			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, TexCoords));

			glEnableVertexAttribArray(3);
			glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, Tangent));

			glEnableVertexAttribArray(4);
			glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, Bitangent));

			glBindVertexArray(0);
		}


		void Mesh::Draw(const Shader& shader, bool cullBackfaces, bool uploadMaterial, const AssetHandle<Material>& materialOverride) const
		{
			Material* mat = materialOverride.IsValid() ? GetAssetManager().Get(materialOverride) : m_material.get();
			if (uploadMaterial) {
				if (mat != nullptr) {
					ENGINE_GLCheckError();


					if (mat->GetDiffuseTexture().IsValid() && GetAssetManager().Get(mat->GetDiffuseTexture()) != nullptr) {
						shader.SetInt("diffuseTexture", 0);
						GetAssetManager().Get(mat->GetDiffuseTexture())->Bind(0);
					}
					ENGINE_GLCheckError();
					if (mat->GetNormalTexture().IsValid() && GetAssetManager().Get(mat->GetNormalTexture()) != nullptr) {
						shader.SetInt("normalTexture", 1);
						GetAssetManager().Get(mat->GetNormalTexture())->Bind(1);
					}
					ENGINE_GLCheckError();
					if (mat->GetSpecularTexture().IsValid() && GetAssetManager().Get(mat->GetSpecularTexture()) != nullptr) {
						shader.SetInt("specularTexture", 2);
						GetAssetManager().Get(mat->GetSpecularTexture())->Bind(2);
					}
					ENGINE_GLCheckError();

					shader.SetInt("hasDiffuseTexture", mat->GetDiffuseTexture().IsValid() ? 1 : 0);
					shader.SetInt("hasNormalTexture", mat->GetNormalTexture().IsValid() ? 1 : 0);
					shader.SetInt("hasSpecularTexture", mat->GetSpecularTexture().IsValid() ? 1 : 0);

					shader.SetVec2("textureScale", mat->GetTextureScale());


					shader.SetVec3("uDiffuseColor", mat->GetDiffuseColor());
					shader.SetVec3("uSpecularColor", mat->GetSpecularColor());
					shader.SetVec3("uAmbientColor", mat->GetAmbientColor());
					shader.SetVec3("uEmissiveColor", mat->GetEmissiveColor());
					shader.SetFloat("uShininess", mat->GetShininess());
				}
				else {
					shader.SetInt("hasDiffuseTexture", 0);
					shader.SetInt("hasNormalTexture", 0);
					shader.SetInt("hasSpecularTexture", 0);
				}
			}
			if (cullBackfaces)
				glEnable(GL_CULL_FACE);
			else
				glDisable(GL_CULL_FACE);


			glBindVertexArray(m_vao);
			glDrawElements(GL_TRIANGLES, static_cast<int>(m_indices.size()), GL_UNSIGNED_INT, nullptr);
			glBindVertexArray(0);
			ENGINE_GLCheckError();
			if (uploadMaterial) {
				if (mat != nullptr) {
					if (mat->GetDiffuseTexture().IsValid() && GetAssetManager().Get(mat->GetDiffuseTexture()) != nullptr) {
						glActiveTexture(GL_TEXTURE0);
						Texture::Unbind();
					}
					ENGINE_GLCheckError();
					if (mat->GetNormalTexture().IsValid() && GetAssetManager().Get(mat->GetNormalTexture()) != nullptr) {
						glActiveTexture(GL_TEXTURE0);
						Texture::Unbind();
					}
					ENGINE_GLCheckError();
					if (mat->GetSpecularTexture().IsValid() && GetAssetManager().Get(mat->GetSpecularTexture()) != nullptr) {
						glActiveTexture(GL_TEXTURE0);
						Texture::Unbind();
					}
					ENGINE_GLCheckError();
				}
				else {
				}
			}
		}

		[[maybe_unused]] void Mesh::CleanUp()
		{
			if (m_vao) {
				glDeleteVertexArrays(1, &m_vao);
				s_vaos.erase(m_vao);
				m_vao = 0;
			}
			if (m_vbo) {
				glDeleteBuffers(1, &m_vbo);
				s_vbos.erase(m_vbo);
				m_vbo = 0;
			}
			if (m_ebo) {
				glDeleteBuffers(1, &m_ebo);
				s_ebos.erase(m_ebo);
				m_ebo = 0;
			}
		}

		[[maybe_unused]] void Mesh::CleanAllMeshes()
		{
			GetDefaultLogger()->info("Cleaning up all VAOs, VBOs, and EBOs");

			for (GLuint vao : s_vaos) {
				glDeleteVertexArrays(1, &vao);
			}
			for (GLuint vbo : s_vbos) {
				glDeleteBuffers(1, &vbo);
			}
			for (GLuint ebo : s_ebos) {
				glDeleteBuffers(1, &ebo);
			}

			s_vaos.clear();
			s_vbos.clear();
			s_ebos.clear();
		}
	} // namespace Rendering
} // namespace Engine


#include "assets/AssetManager.inl"