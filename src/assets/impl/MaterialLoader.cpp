//
// Created by gabe on 8/22/25.
//

#include "MaterialLoader.h"
#include <string>
#include <fstream>
#include <stdexcept>
#include <vector>
#include <nlohmann/json.hpp>
#include <glm/vec3.hpp>

namespace Engine {

	std::unique_ptr<Material> MaterialLoader::LoadFromFile(const std::string& path)
	{
		std::unique_ptr<Material> mat = std::make_unique<Material>();
		std::ifstream             file(path);
		if (!file.is_open()) throw std::runtime_error("Failed to open material file: " + path);

		nlohmann::json j;
		file >> j;


		mat->SetName(j.value("name", ""));

		if (j.contains("textures")) {
			mat->SetDiffuseTexture(AssetHandle<Texture>(j["textures"].value("diffuse", "")));
			mat->SetSpecularTexture(AssetHandle<Texture>(j["textures"].value("specular", "")));
			mat->SetNormalTexture(AssetHandle<Texture>(j["textures"].value("normal", "")));
			mat->SetHeightTexture(AssetHandle<Texture>(j["textures"].value("height", "")));
		}

		if (j.contains("colors")) {
			if (j["colors"].contains("diffuse")) {
				auto arr = j["colors"]["diffuse"];
				if (arr.is_array() && arr.size() == 3) {
					mat->SetDiffuseColor(glm::vec3(arr[0].get<float>(), arr[1].get<float>(), arr[2].get<float>()));
				}
			}
			if (j["colors"].contains("specular")) {
				auto arr = j["colors"]["specular"];
				if (arr.is_array() && arr.size() == 3) {
					mat->SetSpecularColor(glm::vec3(arr[0].get<float>(), arr[1].get<float>(), arr[2].get<float>()));
				}
			}
			if (j["colors"].contains("ambient")) {
				auto arr = j["colors"]["ambient"];
				if (arr.is_array() && arr.size() == 3) {
					mat->SetAmbientColor(glm::vec3(arr[0].get<float>(), arr[1].get<float>(), arr[2].get<float>()));
				}
			}
			if (j["colors"].contains("emissive")) {
				auto arr = j["colors"]["emissive"];
				if (arr.is_array() && arr.size() == 3) {
					mat->SetEmissiveColor(glm::vec3(arr[0].get<float>(), arr[1].get<float>(), arr[2].get<float>()));
				}
			}
		}

		if (j.contains("properties")) {
			mat->SetShininess(j["properties"].value("shininess", 0.0f));
		}

		return mat;
	}

	void MaterialLoader::SaveMaterial(const Material& mat, const std::string& path)
	{
		nlohmann::json j;
		j["name"]       = mat.GetName();
		j["textures"]   = {{"diffuse", mat.GetDiffuseTexture().GetID()}, {"specular", mat.GetSpecularTexture().GetID()}, {"normal", mat.GetNormalTexture().GetID()}, {"height", mat.GetHeightTexture().GetID()}};
		j["colors"]     = {{"diffuse", {mat.GetDiffuseColor().r, mat.GetDiffuseColor().g, mat.GetDiffuseColor().b}},
		                   {"specular", {mat.GetSpecularColor().r, mat.GetSpecularColor().g, mat.GetSpecularColor().b}},
		                   {"ambient", {mat.GetAmbientColor().r, mat.GetAmbientColor().g, mat.GetAmbientColor().b}},
		                   {"emissive", {mat.GetEmissiveColor().r, mat.GetEmissiveColor().g, mat.GetEmissiveColor().b}}};
		j["properties"] = {
		    {"shininess", mat.GetShininess()},
		};

		namespace fs = std::filesystem;
		fs::path filePath(path);
		if (filePath.has_parent_path()) {
			fs::create_directories(filePath.parent_path());
		}

		std::ofstream file(path);
		if (!file.is_open()) throw std::runtime_error("Failed to open material file for writing: " + path);
		file << j.dump(4);
	}
} // namespace Engine