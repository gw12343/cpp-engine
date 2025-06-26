#include "ModelLoader.h"

#include "rendering/Texture.h"
#include "Utils.h"

#include <filesystem>

namespace Engine {
	namespace Rendering {

		std::shared_ptr<Model> ModelLoader::LoadModel(const std::string& path)
		{
			Assimp::Importer importer;
			const aiScene*   scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_CalcTangentSpace | aiProcess_FlipUVs);


			if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
				spdlog::critical("ASSIMP ERROR::{}", importer.GetErrorString());
				return nullptr;
			}

			ENGINE_VERIFY(scene, "Assimp failed to load scene");
			ENGINE_VERIFY(!(scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE), "Assimp loaded incomplete scene");
			ENGINE_VERIFY(scene->mRootNode, "Assimp scene missing root node");


			std::vector<std::shared_ptr<Mesh>> meshes;
			ProcessNode(scene->mRootNode, scene, meshes, std::filesystem::path(path).parent_path().string());

			auto model = std::make_shared<Model>();
			ENGINE_ASSERT(model, "Failed to allocate Model");
			model->m_meshes = meshes;
			return model;
		}

		void ModelLoader::ProcessNode(aiNode* node, const aiScene* scene, std::vector<std::shared_ptr<Mesh>>& meshes, const std::string& directory)
		{
			ENGINE_ASSERT(node, "Null aiNode passed to ProcessNode");
			ENGINE_ASSERT(scene, "Null aiScene passed to ProcessNode");

			for (unsigned int i = 0; i < node->mNumMeshes; i++) {
				ENGINE_ASSERT(node->mMeshes[i] < scene->mNumMeshes, "Invalid mesh index in node");
				aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
				ENGINE_ASSERT(mesh, "Null aiMesh pointer encountered");
				meshes.push_back(ProcessMesh(mesh, scene, directory));
			}

			for (unsigned int i = 0; i < node->mNumChildren; i++) {
				ProcessNode(node->mChildren[i], scene, meshes, directory);
			}
		}

		std::shared_ptr<Mesh> ModelLoader::ProcessMesh(aiMesh* mesh, const aiScene* scene, const std::string& directory)
		{
			ENGINE_ASSERT(mesh, "Null mesh passed to ProcessMesh");

			std::vector<Vertex>       vertices;
			std::vector<unsigned int> indices;

			for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
				Vertex vertex;
				vertex.Position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);

				if (mesh->mNormals) {
					vertex.Normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
				}

				if (mesh->mTextureCoords[0]) {
					vertex.TexCoords = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
				}
				else {
					vertex.TexCoords = glm::vec2(0.0f, 0.0f);
				}

				if (mesh->mTangents) {
					vertex.Tangent = glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
				}

				if (mesh->mBitangents) {
					vertex.Bitangent = glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z);
				}

				vertices.push_back(vertex);
			}

			for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
				aiFace face = mesh->mFaces[i];
				ENGINE_ASSERT(face.mNumIndices >= 3, "Face does not contain at least 3 indices");
				for (unsigned int j = 0; j < face.mNumIndices; j++) {
					indices.push_back(face.mIndices[j]);
				}
			}

			std::shared_ptr<Material> material;
			if (mesh->mMaterialIndex >= 0) {
				ENGINE_ASSERT(scene, "Scene is null when accessing materials");
				ENGINE_ASSERT(mesh->mMaterialIndex < scene->mNumMaterials, "Invalid material index in mesh");
				aiMaterial* aiMat = scene->mMaterials[mesh->mMaterialIndex];
				material          = LoadMaterial(aiMat, directory);
			}
			else {
				material = std::make_shared<Material>();
				ENGINE_ASSERT(material, "Failed to create default material");
			}

			auto result = std::make_shared<Mesh>(vertices, indices, material);
			ENGINE_ASSERT(result, "Failed to create mesh");
			return result;
		}

		std::shared_ptr<Material> ModelLoader::LoadMaterial(aiMaterial* mat, const std::string& directory)
		{
			ENGINE_ASSERT(mat, "Null aiMaterial passed to LoadMaterial");

			auto material = std::make_shared<Material>();
			ENGINE_ASSERT(material, "Failed to create Material");

			aiColor3D color(0.f, 0.f, 0.f);
			if (mat->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) material->SetDiffuseColor(glm::vec3(color.r, color.g, color.b));
			if (mat->Get(AI_MATKEY_COLOR_SPECULAR, color) == AI_SUCCESS) material->SetSpecularColor(glm::vec3(color.r, color.g, color.b));
			if (mat->Get(AI_MATKEY_COLOR_AMBIENT, color) == AI_SUCCESS) material->SetAmbientColor(glm::vec3(color.r, color.g, color.b));
			if (mat->Get(AI_MATKEY_COLOR_EMISSIVE, color) == AI_SUCCESS) material->SetEmissiveColor(glm::vec3(color.r, color.g, color.b));

			float shininess;
			if (mat->Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS) material->SetShininess(shininess);

			auto diffuseTextures = LoadMaterialTextures(mat, aiTextureType_DIFFUSE, "texture_diffuse", directory);
			if (!diffuseTextures.empty()) material->SetDiffuseTexture(diffuseTextures[0]);

			auto specularTextures = LoadMaterialTextures(mat, aiTextureType_SPECULAR, "texture_specular", directory);
			if (!specularTextures.empty()) material->SetSpecularTexture(specularTextures[0]);

			auto normalTextures = LoadMaterialTextures(mat, aiTextureType_NORMALS, "texture_normal", directory);
			if (!normalTextures.empty()) material->SetNormalTexture(normalTextures[0]);

			auto heightTextures = LoadMaterialTextures(mat, aiTextureType_HEIGHT, "texture_height", directory);
			if (!heightTextures.empty()) material->SetHeightTexture(heightTextures[0]);

			material->SetName("MaterialYay");

			return material;
		}

		std::vector<std::shared_ptr<Texture>> ModelLoader::LoadMaterialTextures(aiMaterial* mat, aiTextureType type, const std::string& typeName, const std::string& directory)
		{
			ENGINE_ASSERT(mat, "Null aiMaterial passed to LoadMaterialTextures");

			std::vector<std::shared_ptr<Texture>> textures;
			for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
				aiString str;
				if (mat->GetTexture(type, i, &str) != AI_SUCCESS) {
					spdlog::warn("Failed to get texture {} from material", i);
					continue;
				}

				std::string filename = str.C_Str();
				auto        texture  = std::make_shared<Texture>();
				ENGINE_ASSERT(texture, "Failed to allocate Texture");

				std::string newPath = filename.substr(0, 1) == "/" ? filename : (directory + "/" + filename);

				if (texture->LoadFromFile(newPath)) {
					textures.push_back(texture);
				}
				else {
					spdlog::warn("Failed to load texture from file: {}", filename);
					spdlog::error("e: {}", directory);
				}
			}
			return textures;
		}

	} // namespace Rendering
} // namespace Engine