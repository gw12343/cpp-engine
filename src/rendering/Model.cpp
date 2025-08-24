#include "Model.h"

#include "assets/impl/ModelLoader.h"
#include "utils/Utils.h"


namespace Engine::Rendering {
	void Model::Draw(const Shader& shader, bool uploadMaterial) const
	{
		for (const auto& mesh : m_meshes) {
			mesh->Draw(shader, uploadMaterial, AssetHandle<Material>());
		}
	}

	void Model::Draw(const Shader& shader, bool uploadMaterial, const std::vector<AssetHandle<Material>> materialOverrides) const
	{
		//		std::vector<AssetHandle<Material>> newMats;
		//		// TODO meh ts bad
		//		if (materialOverrides.size() > 0) {
		//			for (int i = 0; i < std::min(m_meshes.size(), materialOverrides.size()); i++) {
		//				newMats.push_back(materialOverrides[i]);
		//			}
		//		}
		//		while (newMats.size() < m_meshes.size()) {
		//			newMats.push_back(AssetHandle<Material>());
		//		}
		//
		//		ENGINE_ASSERT(newMats.size() == m_meshes.size(), "Failed to chose materials for mesh");
		ENGINE_ASSERT(materialOverrides.size() == m_meshes.size(), "Invalid material list size");

		int j = 0;
		for (const auto& mesh : m_meshes) {
			mesh->Draw(shader, uploadMaterial, materialOverrides[j++]);
		}
	}
} // namespace Engine::Rendering
