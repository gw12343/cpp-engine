#include "Model.h"

#include "assets/impl/ModelLoader.h"


namespace Engine::Rendering {
	void Model::Draw(const Shader& shader, const AssetHandle<Material>& materialOverride) const
	{
		for (const auto& mesh : m_meshes) {
			mesh->Draw(shader, materialOverride);
		}
	}
} // namespace Engine::Rendering
