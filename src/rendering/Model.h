#pragma once

#include "Mesh.h"
#include "Shader.h"
#include <vector>
#include <string>
#include <memory>

namespace Engine {
class Model {
public:
    Model() = default;
    ~Model() = default;

    void Draw(const Shader& shader) const;
    const std::vector<std::shared_ptr<Mesh>>& GetMeshes() const { return m_meshes; }

private:
    friend class ModelLoader;
    
    std::vector<std::shared_ptr<Mesh>> m_meshes;
    std::string m_directory;

    void LoadModel(const std::string& path);
}; 
}