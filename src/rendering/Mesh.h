#pragma once

#include "Material.h"
#include "Texture.h"
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include "Shader.h"
#include <glad/glad.h>

namespace Engine {
    struct Vertex {
        glm::vec3 Position;
        glm::vec3 Normal;
        glm::vec2 TexCoords;
        glm::vec3 Tangent;
        glm::vec3 Bitangent;
    };

    class Mesh {
    public:
        Mesh(const std::vector<Vertex>& vertices,
            const std::vector<unsigned int>& indices,
            const std::shared_ptr<Material>& material);
        ~Mesh();

        void Draw(const Shader& shader) const;
        const std::shared_ptr<Material>& GetMaterial() const { return m_material; }

    private:
        void SetupMesh();

        std::vector<Vertex> m_vertices;
        std::vector<unsigned int> m_indices;
        std::shared_ptr<Material> m_material;

        // OpenGL objects
        GLuint m_vao;
        GLuint m_vbo;
        GLuint m_ebo;
    }; 
}