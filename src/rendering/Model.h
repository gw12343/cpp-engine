#pragma once

#include "Mesh.h"
#include "Shader.h"

#include <memory>
#include <string>
#include <vector>


namespace Engine::Rendering {
	class Model {
	  public:
		Model()  = default;
		~Model() = default;

		void Draw(const Shader& shader, const AssetHandle<Material>& materialOverride) const;

		[[maybe_unused]] [[nodiscard]] const std::vector<std::shared_ptr<Mesh>>& GetMeshes() const { return m_meshes; }

		glm::vec3 m_boundsMin{};

		glm::vec3 m_boundsMax{};

	  private:
		friend class ModelLoader;

		std::vector<std::shared_ptr<Mesh>> m_meshes;
		std::string                        m_directory;
	};
} // namespace Engine::Rendering
