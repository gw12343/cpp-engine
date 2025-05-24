#pragma once

#include "rendering/Material.h"
#include "rendering/Mesh.h"
#include "rendering/Model.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <string>
#include <unordered_map>

namespace Engine {
	namespace Rendering {
		class ModelLoader {
		  public:
			static std::shared_ptr<Model> LoadModel(const std::string& path);

		  private:
			static void                                  ProcessNode(aiNode* node, const aiScene* scene, std::vector<std::shared_ptr<Mesh>>& meshes, const std::string& directory);
			static std::shared_ptr<Mesh>                 ProcessMesh(aiMesh* mesh, const aiScene* scene, const std::string& directory);
			static std::shared_ptr<Material>             LoadMaterial(aiMaterial* mat, const std::string& directory);
			static std::vector<std::shared_ptr<Texture>> LoadMaterialTextures(aiMaterial* mat, aiTextureType type, const std::string& typeName, const std::string& directory);
		};
	} // namespace Rendering
} // namespace Engine