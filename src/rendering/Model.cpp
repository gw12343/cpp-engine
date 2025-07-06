#include "Model.h"

#include "assets/impl/ModelLoader.h"


namespace Engine::Rendering {
	void Model::Draw(const Shader& shader) const
	{
		for (const auto& mesh : m_meshes) {
			mesh->Draw(shader);
		}
	}
} // namespace Engine::Rendering
