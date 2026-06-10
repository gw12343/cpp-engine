#include "Model.h"

#include "assets/impl/ModelLoader.h"



namespace Engine::Rendering {
	void Model::Draw(const Shader& shader, bool cullBackfaces, bool uploadMaterial) const
	{
		for (const auto& mesh : m_meshes) {
			mesh->Draw(shader, cullBackfaces, uploadMaterial, MaterialHandle());
		}
	}

	void Model::Draw(const Shader& shader, bool cullBackfaces, bool uploadMaterial, const std::vector<MaterialHandle>& materialOverrides) const
	{
		ENGINE_VERIFY(materialOverrides.size() == m_meshes.size(), "Invalid material list size");

		int j = 0;
		for (const auto& mesh : m_meshes) {
			mesh->Draw(shader, cullBackfaces, uploadMaterial, materialOverrides[j++]);
		}
	}
} // namespace Engine::Rendering
