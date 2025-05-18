#include "Mesh.h"
#include "Shader.h"
#include <glad/glad.h>
#include <spdlog/spdlog.h>

namespace Engine {
Mesh::Mesh(const std::vector<Vertex>& vertices,
           const std::vector<unsigned int>& indices,
           const std::shared_ptr<Material>& material)
    : m_vertices(vertices)
    , m_indices(indices)
    , m_material(material)
    , m_vao(0)
    , m_vbo(0)
    , m_ebo(0) {
    SetupMesh();
}

Mesh::~Mesh() {
    spdlog::debug("Deleting mesh ");
    glDeleteVertexArrays(1, &m_vao);
    glDeleteBuffers(1, &m_vbo);
    glDeleteBuffers(1, &m_ebo);
}

void Mesh::SetupMesh() {
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);

    glBindVertexArray(m_vao);

    // Load vertex data
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(Vertex), &m_vertices[0], GL_STATIC_DRAW);

    // Load index data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof(unsigned int), &m_indices[0], GL_STATIC_DRAW);

    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

    // Normal attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));

    // Texture coordinate attribute
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));

    // Tangent attribute
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));

    // Bitangent attribute
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Bitangent));

    glBindVertexArray(0);
}

void Mesh::Draw(const Shader& shader) const {
    // Bind material textures
    if (m_material->GetDiffuseTexture()) {
        glActiveTexture(GL_TEXTURE0);
        shader.SetInt("material.diffuse", 0);
        m_material->GetDiffuseTexture()->Bind(0);
    }

    if (m_material->GetSpecularTexture()) {
        glActiveTexture(GL_TEXTURE1);
        shader.SetInt("material.specular", 1);
        m_material->GetSpecularTexture()->Bind(1);
    }

    if (m_material->GetNormalTexture()) {
        glActiveTexture(GL_TEXTURE2);
        shader.SetInt("material.normal", 2);
        m_material->GetNormalTexture()->Bind(2);
    }

    // Set material colors
    shader.SetVec3("material.diffuseColor", m_material->GetDiffuseColor());
    shader.SetVec3("material.specularColor", m_material->GetSpecularColor());
    shader.SetVec3("material.ambientColor", m_material->GetAmbientColor());
    shader.SetVec3("material.emissiveColor", m_material->GetEmissiveColor());
    shader.SetFloat("material.shininess", m_material->GetShininess());

    // Draw mesh
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, m_indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    // Unbind all textures
    if (m_material->GetNormalTexture()) {
        glActiveTexture(GL_TEXTURE2);
        m_material->GetNormalTexture()->Unbind();
    }
    
    if (m_material->GetSpecularTexture()) {
        glActiveTexture(GL_TEXTURE1);
        m_material->GetSpecularTexture()->Unbind();
    }
    
    if (m_material->GetDiffuseTexture()) {
        glActiveTexture(GL_TEXTURE0);
        m_material->GetDiffuseTexture()->Unbind();
    }

    // Reset active texture unit to default
    glActiveTexture(GL_TEXTURE0);
} 
}