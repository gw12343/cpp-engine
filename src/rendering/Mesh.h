#pragma once

#include "Material.h"
#include "Shader.h"
#include "Texture.h"


#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>


namespace Engine::Rendering {
	struct Vertex {
		glm::vec3 Position;
		glm::vec3 Normal;
		glm::vec2 TexCoords;
		glm::vec3 Tangent;
		glm::vec3 Bitangent;
	};

	class Mesh {
	  public:
		Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices, const std::shared_ptr<Material>& material);
		~Mesh() = default;

		void                                           Draw(const Shader& shader, bool uploadMaterial, const AssetHandle<Material>& materialOverride) const;
		[[maybe_unused]] void                          CleanUp();
		[[maybe_unused]] static void                   CleanAllMeshes();
		[[nodiscard]] const std::shared_ptr<Material>& GetMaterial() const { return m_material; }

	  private:
		void SetupMesh();

		std::vector<Vertex>       m_vertices;
		std::vector<unsigned int> m_indices;
		std::shared_ptr<Material> m_material;


	  private:
		GLuint m_vao;
		GLuint m_vbo;
		GLuint m_ebo;

		static std::unordered_set<GLuint> s_vaos;
		static std::unordered_set<GLuint> s_vbos;
		static std::unordered_set<GLuint> s_ebos;
	};
} // namespace Engine::Rendering
