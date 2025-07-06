#include "ModelLoader.h"

#include "rendering/Texture.h"
#include "utils/Utils.h"
#include "assets/AssetManager.h"
#include "core/EngineData.h"
#include "glm/gtc/type_ptr.hpp"

#include <filesystem>
#include <iostream>

namespace Engine {
	namespace Rendering {

		std::unique_ptr<Model> ModelLoader::LoadFromFile(const std::string& path)
		{
			Assimp::Importer importer;
			const aiScene*   scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_CalcTangentSpace | aiProcess_FlipUVs);

			if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
				spdlog::critical("ASSIMP ERROR: {}", importer.GetErrorString());
				return nullptr;
			}

			std::vector<std::shared_ptr<Mesh>> meshes;
			std::string                        directory = std::filesystem::path(path).parent_path().string();
			glm::vec3                          boundsMin(std::numeric_limits<float>::max());
			glm::vec3                          boundsMax(std::numeric_limits<float>::lowest());

			ProcessNode(scene->mRootNode, scene, meshes, directory, glm::mat4(1.0f), boundsMin, boundsMax);

			auto model = std::make_unique<Model>();
			ENGINE_ASSERT(model, "Failed to allocate Model");
			model->m_meshes    = std::move(meshes);
			model->m_boundsMin = boundsMin;
			model->m_boundsMax = boundsMax;
			return model;
		}

		void ModelLoader::ProcessNode(aiNode* node, const aiScene* scene, std::vector<std::shared_ptr<Mesh>>& meshes, const std::string& directory, const glm::mat4& parentTransform, glm::vec3& boundsMin, glm::vec3& boundsMax)
		{
			ENGINE_ASSERT(node && scene, "Invalid parameters passed to ProcessNode");

			glm::mat4 transform = parentTransform * glm::transpose(glm::make_mat4(&node->mTransformation.a1));

			for (unsigned int i = 0; i < node->mNumMeshes; i++) {
				ENGINE_ASSERT(node->mMeshes[i] < scene->mNumMeshes, "Invalid mesh index in node");
				aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
				ENGINE_ASSERT(mesh, "Null aiMesh pointer encountered");

				// Bounding box accumulation
				for (unsigned int v = 0; v < mesh->mNumVertices; ++v) {
					aiVector3D pos         = mesh->mVertices[v];
					glm::vec4  transformed = transform * glm::vec4(pos.x, pos.y, pos.z, 1.0f);
					glm::vec3  p           = glm::vec3(transformed);

					boundsMin = glm::min(boundsMin, p);
					boundsMax = glm::max(boundsMax, p);
				}

				meshes.push_back(ProcessMesh(mesh, scene, directory));
			}

			for (unsigned int i = 0; i < node->mNumChildren; i++) {
				ProcessNode(node->mChildren[i], scene, meshes, directory, transform, boundsMin, boundsMax);
			}
		}


		std::shared_ptr<Mesh> ModelLoader::ProcessMesh(aiMesh* mesh, const aiScene* scene, const std::string& directory)
		{
			ENGINE_ASSERT(mesh, "Null mesh passed to ProcessMesh");

			std::vector<Vertex> vertices;
			vertices.reserve(mesh->mNumVertices);

			for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
				Vertex vertex;
				vertex.Position  = {mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z};
				vertex.Normal    = mesh->mNormals ? glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z) : glm::vec3(0.0f);
				vertex.TexCoords = mesh->mTextureCoords[0] ? glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y) : glm::vec2(0.0f);
				vertex.Tangent   = mesh->mTangents ? glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z) : glm::vec3(0.0f);
				vertex.Bitangent = mesh->mBitangents ? glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z) : glm::vec3(0.0f);
				vertices.push_back(vertex);
			}

			std::vector<unsigned int> indices;
			for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
				aiFace face = mesh->mFaces[i];
				ENGINE_ASSERT(face.mNumIndices >= 3, "Face does not contain at least 3 indices");
				indices.insert(indices.end(), face.mIndices, face.mIndices + face.mNumIndices);
			}

			std::shared_ptr<Material> material;
			if (mesh->mMaterialIndex >= 0) {
				ENGINE_ASSERT(mesh->mMaterialIndex < scene->mNumMaterials, "Invalid material index");
				aiMaterial* aiMat = scene->mMaterials[mesh->mMaterialIndex];
				material          = LoadMaterial(aiMat, directory);
			}
			else {
				material = std::make_shared<Material>();
				ENGINE_ASSERT(material, "Failed to create fallback material");
			}

			return std::make_shared<Mesh>(vertices, indices, material);
		}

		std::vector<AssetHandle<Texture>> ModelLoader::LoadMaterialTextures(aiMaterial* mat, aiTextureType type, const std::string& typeName, const std::string& directory)
		{
			ENGINE_ASSERT(mat, "Null aiMaterial passed to LoadMaterialTextures");

			std::vector<AssetHandle<Texture>> textures;

			for (unsigned int i = 0; i < mat->GetTextureCount(type); ++i) {
				aiString str;
				if (mat->GetTexture(type, i, &str) != AI_SUCCESS) {
					spdlog::warn("Failed to get texture #{} from material", i);
					continue;
				}

				std::string rawStr = str.C_Str();

				// Replace backslashes with slashes
				std::replace(rawStr.begin(), rawStr.end(), '\\', '/');

				std::filesystem::path oldPath(rawStr);
				std::filesystem::path fullPath = std::filesystem::path("resources/textures") / oldPath.filename();

				std::string fullPathStr = fullPath.string();

				spdlog::info("Loading texture: {} (type: {})", fullPathStr, typeName);

				auto handle = GetAssetManager().Load<Texture>(fullPathStr);
				if (handle.IsValid()) {
					textures.push_back(handle);
				}
				else {
					spdlog::warn("Failed to load {} texture from path: {}", typeName, fullPathStr);
				}
			}

			return textures;
		}
		std::shared_ptr<Material> ModelLoader::LoadMaterial(aiMaterial* mat, const std::string& directory)
		{
			ENGINE_ASSERT(mat, "Null aiMaterial passed to LoadMaterial");

			auto material = std::make_shared<Material>();
			ENGINE_ASSERT(material, "Failed to create Material");

			aiColor3D color(0.f, 0.f, 0.f);
			float     shininess = 0.0f;

			if (mat->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) material->SetDiffuseColor({color.r, color.g, color.b});
			if (mat->Get(AI_MATKEY_COLOR_SPECULAR, color) == AI_SUCCESS) material->SetSpecularColor({color.r, color.g, color.b});
			if (mat->Get(AI_MATKEY_COLOR_AMBIENT, color) == AI_SUCCESS) material->SetAmbientColor({color.r, color.g, color.b});
			if (mat->Get(AI_MATKEY_COLOR_EMISSIVE, color) == AI_SUCCESS) material->SetEmissiveColor({color.r, color.g, color.b});
			if (mat->Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS) material->SetShininess(shininess);

			auto loadFirst = [&](aiTextureType type, const std::string& name, auto setter) {
				auto textures = LoadMaterialTextures(mat, type, name, directory);
				if (!textures.empty()) setter(material, textures[0]);
			};

			loadFirst(aiTextureType_DIFFUSE, "texture_diffuse", [](auto& m, auto& t) { m->SetDiffuseTexture(t); });
			loadFirst(aiTextureType_SPECULAR, "texture_specular", [](auto& m, auto& t) { m->SetSpecularTexture(t); });
			loadFirst(aiTextureType_NORMALS, "texture_normal", [](auto& m, auto& t) { m->SetNormalTexture(t); });
			loadFirst(aiTextureType_HEIGHT, "texture_height", [](auto& m, auto& t) { m->SetHeightTexture(t); });

			material->SetName("Material");
			return material;
		}

	} // namespace Rendering
} // namespace Engine

#include "assets/AssetManager.inl"
