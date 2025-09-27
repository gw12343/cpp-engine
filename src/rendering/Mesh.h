#pragma once

#include "Material.h"
#include "Shader.h"
#include "Texture.h"

#include "Jolt/Jolt.h"
#include "Jolt/Physics/Collision/Shape/Shape.h"


#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

using namespace JPH;


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

		void                                           Draw(const Shader& shader, bool cullBackfaces, bool uploadMaterial, const AssetHandle<Material>& materialOverride) const;
		[[maybe_unused]] void                          CleanUp();
		[[maybe_unused]] static void                   CleanAllMeshes();
		[[nodiscard]] const std::shared_ptr<Material>& GetMaterial() const { return m_material; }
		[[nodiscard]] std::vector<Vertex>              GetVertices() const { return m_vertices; }
		[[nodiscard]] std::vector<unsigned int>        GetIndices() const { return m_indices; }
		[[nodiscard]] RefConst<JPH::Shape>             GetCollisionShape() const { return m_collisionShape; }

	  private:
		void SetupMesh();

		std::vector<Vertex>       m_vertices;
		std::vector<unsigned int> m_indices;
		std::shared_ptr<Material> m_material;

		// TODO dont store for all meshes
		RefConst<JPH::Shape> m_collisionShape;

	  private:
		GLuint m_vao;
		GLuint m_vbo;
		GLuint m_ebo;

		static std::unordered_set<GLuint> s_vaos;
		static std::unordered_set<GLuint> s_vbos;
		static std::unordered_set<GLuint> s_ebos;
	};
} // namespace Engine::Rendering
