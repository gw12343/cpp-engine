#include "Model.h"
#include "utils/ModelLoader.h"

namespace Engine {
void Model::Draw(const Shader& shader) const {
    for (const auto& mesh : m_meshes) {
        mesh->Draw(shader);
    }
}
}