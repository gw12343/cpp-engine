#include "Model.h"

#include "assets/ModelLoader.h"


namespace Engine::Rendering {
	void Model::Draw(const Shader& shader) const
	{
		for (const auto& mesh : m_meshes) {
			mesh->Draw(shader);
		}
	}
	[[maybe_unused]] void Model::LoadModel(const std::string& path)
	{
	}
} // namespace Engine::Rendering
