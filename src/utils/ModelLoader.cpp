#include "ModelLoader.h"

#include "rendering/Texture.h"

#include <filesystem>
#include <stb/stb_image.h>
namespace Engine {

	std::shared_ptr<Model> ModelLoader::LoadModel(const std::string& path)
	{
		Assimp::Importer importer;
		const aiScene*   scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_CalcTangentSpace | aiProcess_FlipUVs);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
			spdlog::critical("ASSIMP ERROR::{}", importer.GetErrorString());
			return nullptr;
		}

		std::vector<std::shared_ptr<Mesh>> meshes;
		ProcessNode(scene->mRootNode, scene, meshes, std::filesystem::path(path).parent_path().string());

		auto model      = std::make_shared<Model>();
		model->m_meshes = meshes;
		return model;
	}

	void ModelLoader::ProcessNode(aiNode* node, const aiScene* scene, std::vector<std::shared_ptr<Mesh>>& meshes, const std::string& directory)
	{
		// Process all the node's meshes
		for (unsigned int i = 0; i < node->mNumMeshes; i++) {
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			meshes.push_back(ProcessMesh(mesh, scene, directory));
		}

		// Then do the same for each of its children
		for (unsigned int i = 0; i < node->mNumChildren; i++) {
			ProcessNode(node->mChildren[i], scene, meshes, directory);
		}
	}

	std::shared_ptr<Mesh> ModelLoader::ProcessMesh(aiMesh* mesh, const aiScene* scene, const std::string& directory)
	{
		std::vector<Vertex>       vertices;
		std::vector<unsigned int> indices;

		// Process vertices
		for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
			Vertex vertex;

			// Position
			vertex.Position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);

			// Normal
			if (mesh->mNormals) {
				vertex.Normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
			}

			// Texture coordinates
			if (mesh->mTextureCoords[0]) {
				vertex.TexCoords = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
			}
			else {
				vertex.TexCoords = glm::vec2(0.0f, 0.0f);
			}

			// Tangent
			if (mesh->mTangents) {
				vertex.Tangent = glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
			}

			// Bitangent
			if (mesh->mBitangents) {
				vertex.Bitangent = glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z);
			}

			vertices.push_back(vertex);
		}

		// Process indices
		for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
			aiFace face = mesh->mFaces[i];
			for (unsigned int j = 0; j < face.mNumIndices; j++) {
				indices.push_back(face.mIndices[j]);
			}
		}

		// Process material
		std::shared_ptr<Material> material;
		if (mesh->mMaterialIndex >= 0) {
			aiMaterial* aiMat = scene->mMaterials[mesh->mMaterialIndex];
			material          = LoadMaterial(aiMat, directory);
		}
		else {
			material = std::make_shared<Material>();
		}

		return std::make_shared<Mesh>(vertices, indices, material);
	}

	std::shared_ptr<Material> ModelLoader::LoadMaterial(aiMaterial* mat, const std::string& directory)
	{
		auto material = std::make_shared<Material>();

		// Load diffuse color
		aiColor3D color(0.f, 0.f, 0.f);
		if (mat->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {
			material->SetDiffuseColor(glm::vec3(color.r, color.g, color.b));
		}

		// Load specular color
		if (mat->Get(AI_MATKEY_COLOR_SPECULAR, color) == AI_SUCCESS) {
			material->SetSpecularColor(glm::vec3(color.r, color.g, color.b));
		}

		// Load ambient color
		if (mat->Get(AI_MATKEY_COLOR_AMBIENT, color) == AI_SUCCESS) {
			material->SetAmbientColor(glm::vec3(color.r, color.g, color.b));
		}

		// Load emissive color
		if (mat->Get(AI_MATKEY_COLOR_EMISSIVE, color) == AI_SUCCESS) {
			material->SetEmissiveColor(glm::vec3(color.r, color.g, color.b));
		}

		// Load shininess
		float shininess;
		if (mat->Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS) {
			material->SetShininess(shininess);
		}
		// Load textures
		auto diffuseTextures = LoadMaterialTextures(mat, aiTextureType_DIFFUSE, "texture_diffuse", directory);
		if (!diffuseTextures.empty()) {
			material->SetDiffuseTexture(diffuseTextures[0]);
		}

		auto specularTextures = LoadMaterialTextures(mat, aiTextureType_SPECULAR, "texture_specular", directory);
		if (!specularTextures.empty()) {
			material->SetSpecularTexture(specularTextures[0]);
		}

		auto normalTextures = LoadMaterialTextures(mat, aiTextureType_NORMALS, "texture_normal", directory);
		if (!normalTextures.empty()) {
			material->SetNormalTexture(normalTextures[0]);
		}

		auto heightTextures = LoadMaterialTextures(mat, aiTextureType_HEIGHT, "texture_height", directory);
		if (!heightTextures.empty()) {
			material->SetHeightTexture(heightTextures[0]);
		}
		material->SetName(mat->GetName().C_Str());
		return material;
	}

	std::vector<std::shared_ptr<Texture>> ModelLoader::LoadMaterialTextures(aiMaterial* mat, aiTextureType type, const std::string& typeName, const std::string& directory)
	{
		std::vector<std::shared_ptr<Texture>> textures;
		for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
			aiString str;
			mat->GetTexture(type, i, &str);
			std::string filename = str.C_Str();

			auto texture = std::make_shared<Texture>();
			if (texture->LoadFromFile(filename)) {
				textures.push_back(texture);
			}
		}
		return textures;
	}
} // namespace Engine