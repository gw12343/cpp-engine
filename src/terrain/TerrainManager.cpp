#include "TerrainManager.h"
#include "utils/Utils.h"
#include "core/EngineData.h"
#include <glad/glad.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>


#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>
#include <stdexcept>

namespace Engine::Terrain {

	void TerrainManager::onInit()
	{
		std::shared_ptr<Engine::Texture> tex1 = std::make_shared<Engine::Texture>();
		tex1->LoadFromFile("resources/textures/Terrain Grass.png");

		std::shared_ptr<Engine::Texture> tex2 = std::make_shared<Engine::Texture>();
		tex2->LoadFromFile("resources/textures/Terrain Dirt.png");

		std::shared_ptr<Engine::Texture> tex3 = std::make_shared<Engine::Texture>();
		tex3->LoadFromFile("resources/textures/Terrain Sand.png");

		std::shared_ptr<Engine::Texture> tex4 = std::make_shared<Engine::Texture>();
		tex4->LoadFromFile("resources/textures/Terrain Rock.png");

		std::shared_ptr<Engine::Texture> tex5 = std::make_shared<Engine::Texture>();
		tex5->LoadFromFile("resources/textures/white.png");


		LoadTerrainFile("resources/terrain/terrain1.bin");
		LoadTerrainFile("resources/terrain/terrain2.bin");
		GenerateMeshes();
		GenerateSplatTextures();
		SetupShaders();


		GetTerrains()[0]->diffuseTextures.push_back(tex1);
		GetTerrains()[0]->diffuseTextures.push_back(tex2);
		GetTerrains()[0]->diffuseTextures.push_back(tex3);
		GetTerrains()[0]->diffuseTextures.push_back(tex4);
		GetTerrains()[0]->diffuseTextures.push_back(tex5);

		GetTerrains()[1]->diffuseTextures.push_back(tex1);
		GetTerrains()[1]->diffuseTextures.push_back(tex2);
		GetTerrains()[1]->diffuseTextures.push_back(tex3);
		GetTerrains()[1]->diffuseTextures.push_back(tex4);
		GetTerrains()[1]->diffuseTextures.push_back(tex5);
	}

	void TerrainManager::onUpdate(float dt)
	{
	}

	void TerrainManager::Render()
	{
		for (auto& tile : terrains) {
			tile->terrainShader->Bind();
			glm::mat4 terrainTransform = glm::translate(glm::mat4(1.0f), glm::vec3(tile->posX, tile->posY, tile->posZ));
			tile->terrainShader->SetMat4("uModel", &terrainTransform);

			glm::mat4 viewM       = GetCamera().GetViewMatrix();
			glm::mat4 projectionM = GetCamera().GetProjectionMatrix();

			tile->terrainShader->SetMat4("uView", &viewM);
			tile->terrainShader->SetMat4("uProjection", &projectionM);

			for (size_t i = 0; i < tile->splatTextures.size(); ++i)
				glBindTextureUnit(static_cast<GLuint>(i), tile->splatTextures[i]);

			size_t base = tile->splatTextures.size();
			for (size_t i = 0; i < tile->diffuseTextures.size(); ++i)
				tile->diffuseTextures[i]->Bind(base + i);

			glBindVertexArray(tile->vao);
			glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(tile->indexCount), GL_UNSIGNED_INT, nullptr);
			glBindVertexArray(0);
		}
	}
	void TerrainManager::onShutdown()
	{
	}

	void TerrainManager::LoadTerrainFile(const std::string& path)
	{
		auto tile = std::make_unique<TerrainTile>();

		std::ifstream file(path, std::ios::binary);
		if (!file) throw std::runtime_error("Cannot open terrain file");

		char magic[4];
		file.read(magic, 4);
		if (std::string(magic, 4) != "TERR") throw std::runtime_error("Invalid format");

		uint32_t version;
		file.read(reinterpret_cast<char*>(&version), sizeof(uint32_t));
		if (version < 2 || version > 3) throw std::runtime_error("Unsupported terrain version");


		uint32_t nameLen;
		file.read(reinterpret_cast<char*>(&nameLen), sizeof(uint32_t));
		tile->name.resize(nameLen);
		file.read(tile->name.data(), nameLen);

		file.read(reinterpret_cast<char*>(&tile->heightRes), sizeof(uint32_t));
		file.read(reinterpret_cast<char*>(&tile->splatRes), sizeof(uint32_t));
		file.read(reinterpret_cast<char*>(&tile->sizeX), sizeof(float));
		file.read(reinterpret_cast<char*>(&tile->sizeY), sizeof(float));
		file.read(reinterpret_cast<char*>(&tile->sizeZ), sizeof(float));

		if (version >= 3) {
			file.read(reinterpret_cast<char*>(&tile->posX), sizeof(float));
			file.read(reinterpret_cast<char*>(&tile->posY), sizeof(float));
			file.read(reinterpret_cast<char*>(&tile->posZ), sizeof(float));
			spdlog::debug("loaded terrain chunk at ({}, {}, {})", tile->posX, tile->posY, tile->posZ);
		}
		else {
			spdlog::debug("unknown position, assuming (0, 0, 0).");
		}

		file.read(reinterpret_cast<char*>(&tile->splatLayerCount), sizeof(uint32_t));

		size_t heightCount = tile->heightRes * tile->heightRes;
		tile->heightmap.resize(heightCount);
		file.read(reinterpret_cast<char*>(tile->heightmap.data()), static_cast<std::streamsize>(heightCount * sizeof(float)));


		size_t splatCount = tile->splatRes * tile->splatRes * tile->splatLayerCount;
		tile->splatmap.resize(splatCount);
		file.read(reinterpret_cast<char*>(tile->splatmap.data()), static_cast<std::streamsize>(splatCount));

		uint32_t treeCount;
		file.read(reinterpret_cast<char*>(&treeCount), sizeof(uint32_t));
		tile->trees.resize(treeCount);
		for (auto& tree : tile->trees) {
			file.read(reinterpret_cast<char*>(&tree.x), sizeof(float));
			file.read(reinterpret_cast<char*>(&tree.y), sizeof(float));
			file.read(reinterpret_cast<char*>(&tree.z), sizeof(float));
			file.read(reinterpret_cast<char*>(&tree.scale), sizeof(float));
			file.read(reinterpret_cast<char*>(&tree.prefabIndex), sizeof(uint32_t));
		}

		terrains.push_back(std::move(tile));
	}


	void TerrainManager::GenerateMeshes()
	{
		for (auto& tile : terrains)
			GenerateMeshForTile(*tile);
	}

	void TerrainManager::SetupShaders()
	{
		for (auto& tile : terrains) {
			ENGINE_ASSERT(tile, "Null tile pointer in SetupShaders");

			std::string vertexCode   = GenerateGLSLVertexShader();
			std::string fragmentCode = GenerateGLSLShader();

			spdlog::debug("VERTEX CODE: \n{}\n FRAGMENT CODE: \n{}", vertexCode, fragmentCode);

			tile->terrainShader = std::make_shared<Engine::Shader>();
			bool success        = tile->terrainShader->LoadFromSource(vertexCode, fragmentCode);
			ENGINE_VERIFY(success, "Failed to compile terrain shader");

			spdlog::debug("num of textures: {}", tile->splatTextures.size());
		}
	}

	void TerrainManager::GenerateSplatTextures()
	{
		for (auto& tile : terrains)
			GenerateSplatTexturesForTile(*tile);
	}


	std::string TerrainManager::GenerateGLSLShader() const
	{
		std::ostringstream ss;
		ss << "#version 420\n";
		ss << "in vec2 vUV;\n";
		ss << "in vec3 vNormal;\n";
		ss << "in vec3 vWorldPos;\n";
		ss << "out vec4 FragColor;\n";

		ss << "uniform vec3 uLightDir = normalize(vec3(1.0, 1.0, 0.5));\n";
		ss << "uniform vec3 uLightColor = vec3(1.0);\n";
		ss << "uniform vec3 uAmbient = vec3(0.3);\n";

		int totalLayers  = (int) terrains[0]->splatLayerCount;
		int textureCount = (totalLayers + 3) / 4;

		for (int i = 0; i < textureCount; ++i)
			ss << "layout(binding = " << i << ") uniform sampler2D splat" << i << ";\n";
		for (int i = 0; i < totalLayers; ++i)
			ss << "layout(binding = " << (textureCount + i) << ") uniform sampler2D tex" << i << ";\n";

		ss << "void main() {\n";
		ss << "  float weights[" << totalLayers << "];\n";
		ss << "  float total = 0.0001;\n";

		for (int i = 0; i < textureCount; ++i) {
			ss << "  vec4 w" << i << " = texture(splat" << i << ", vUV);\n";
			for (int j = 0; j < 4; ++j) {
				int idx = i * 4 + j;
				if (idx >= totalLayers) break;
				ss << "  weights[" << idx << "] = w" << i << "[" << j << "];\n";
				ss << "  total += weights[" << idx << "];\n";
			}
		}

		ss << "  vec4 baseColor = vec4(0.0);\n";
		for (int i = 0; i < totalLayers; ++i) {
			ss << "  baseColor += texture(tex" << i << ", vUV) * (weights[" << i << "] / total);\n";
		}

		ss << "  vec3 N = normalize(vNormal);\n";
		ss << "  vec3 L = normalize(uLightDir);\n";
		ss << "  float NdotL = max(dot(N, L), 0.0);\n";
		ss << "  vec3 lighting = uAmbient + uLightColor * NdotL;\n";

		ss << "  FragColor = vec4(baseColor.rgb * lighting, baseColor.a);\n";
		// ss << "  FragColor = vec4(vNormal.rgb, baseColor.a);\n";
		ss << "}\n";

		return ss.str();
	}


	std::string TerrainManager::GenerateGLSLVertexShader() const
	{
		std::ostringstream ss;
		ss << "#version 420\n";
		ss << "layout(location = 0) in vec3 aPosition;\n";
		ss << "layout(location = 1) in vec2 aUV;\n";
		ss << "layout(location = 2) in vec3 aNormal;\n";
		ss << "uniform mat4 uModel;\n";
		ss << "uniform mat4 uView;\n";
		ss << "uniform mat4 uProjection;\n";
		ss << "out vec2 vUV;\n";
		ss << "out vec3 vNormal;\n";
		ss << "out vec3 vWorldPos;\n";
		ss << "void main() {\n";
		ss << "  vUV = aUV;\n";
		ss << "  vec4 worldPos = uModel * vec4(aPosition, 1.0);\n";
		ss << "  vWorldPos = worldPos.xyz;\n";
		ss << "  vNormal = mat3(transpose(inverse(uModel))) * aNormal;\n"; // transform normal correctly
		ss << "  gl_Position = uProjection * uView * worldPos;\n";
		ss << "}\n";
		return ss.str();
	}


	JPH::Float3 toF3(glm::vec3 v)
	{
		return {v.x, v.y, v.z};
	}


	void TerrainManager::GenerateMeshForTile(TerrainTile& tile)
	{
		uint32_t res = tile.heightRes;

		std::vector<glm::vec3> positions(res * res);
		std::vector<glm::vec2> uvs(res * res);
		std::vector<glm::vec3> normals(res * res, glm::vec3(0.0f));
		std::vector<float>     vertices;
		std::vector<uint32_t>  indices;

		tile.physicsMesh = JPH::TriangleList();

		auto getHeight = [&](int x, int z) -> float {
			x = std::clamp(x, 0, int(res) - 1);
			z = std::clamp(z, 0, int(res) - 1);
			return tile.heightmap[z * res + x];
		};


		// Step 1: Generate vertex positions and UVs
		for (uint32_t z = 0; z < res; ++z) {
			for (uint32_t x = 0; x < res; ++x) {
				auto u = (float) ((float) x / float(res - 1));
				auto v = (float) ((float) z / float(res - 1));
				auto h = (float) (getHeight((int) x, (int) z));

				glm::vec3 pos(tile.sizeX * u, h * tile.sizeY, tile.sizeZ * v);
				positions[z * res + x] = pos;
				uvs[z * res + x]       = glm::vec2(u, v);
			}
		}

		// Step 2: Generate indices, accumulate normals, and populate triangle list for physics
		for (uint32_t z = 0; z < res - 1; ++z) {
			for (uint32_t x = 0; x < res - 1; ++x) {
				uint32_t i0 = z * res + x;
				uint32_t i1 = i0 + 1;
				uint32_t i2 = i0 + res;
				uint32_t i3 = i2 + 1;

				// Triangle 1: i0, i2, i1
				{
					glm::vec3 edge1 = positions[i2] - positions[i0];
					glm::vec3 edge2 = positions[i1] - positions[i0];
					glm::vec3 n     = glm::normalize(glm::cross(edge1, edge2));
					normals[i0] += n;
					normals[i1] += n;
					normals[i2] += n;
					indices.push_back(i0);
					indices.push_back(i2);
					indices.push_back(i1);
					tile.physicsMesh.emplace_back(toF3(positions[i0]), toF3(positions[i2]), toF3(positions[i1]));
				}

				// Triangle 2: i1, i2, i3
				{
					glm::vec3 edge1 = positions[i2] - positions[i1];
					glm::vec3 edge2 = positions[i3] - positions[i1];
					glm::vec3 n     = glm::normalize(glm::cross(edge1, edge2));
					normals[i1] += n;
					normals[i2] += n;
					normals[i3] += n;
					indices.push_back(i1);
					indices.push_back(i2);
					indices.push_back(i3);
					tile.physicsMesh.emplace_back(toF3(positions[i1]), toF3(positions[i2]), toF3(positions[i3]));
				}
			}
		}

		// Step 3: Normalize normals and flatten vertex buffer
		for (uint32_t i = 0; i < positions.size(); ++i) {
			glm::vec3 pos  = positions[i];
			glm::vec2 uv   = uvs[i];
			glm::vec3 norm = glm::normalize(normals[i]);

			vertices.push_back(pos.x);
			vertices.push_back(pos.y);
			vertices.push_back(pos.z);
			vertices.push_back(uv.x);
			vertices.push_back(uv.y);
			vertices.push_back(norm.x);
			vertices.push_back(norm.y);
			vertices.push_back(norm.z);
		}

		tile.indexCount = indices.size();

		glGenVertexArrays(1, &tile.vao);
		glGenBuffers(1, &tile.vbo);
		glGenBuffers(1, &tile.ebo);

		glBindVertexArray(tile.vao);
		glBindBuffer(GL_ARRAY_BUFFER, tile.vbo);
		glBufferData(GL_ARRAY_BUFFER, (GLsizei) vertices.size() * (GLsizei) sizeof(float), vertices.data(), GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tile.ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizei) indices.size() * (GLsizei) sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

		glEnableVertexAttribArray(0); // position
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*) nullptr);
		glEnableVertexAttribArray(1); // uv
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*) (3 * sizeof(float)));
		glEnableVertexAttribArray(2); // normal
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*) (5 * sizeof(float)));

		glBindVertexArray(0);
	}


	void TerrainManager::GenerateSplatTexturesForTile(TerrainTile& tile)
	{
		uint32_t layerCount = tile.splatLayerCount;
		uint32_t res        = tile.splatRes;
		uint32_t count      = (layerCount + 3) / 4;

		std::filesystem::create_directories("debug/splat");

		for (uint32_t i = 0; i < count; ++i) {
			std::vector<uint8_t> rgba(res * res * 4, 0);

			for (uint32_t y = 0; y < res; ++y) {
				for (uint32_t x = 0; x < res; ++x) {
					int idx = (int) (y * res + x) * (int) layerCount;
					for (int c = 0; c < 4; ++c) {
						uint32_t layer = i * 4 + c;
						if (layer < layerCount) rgba[(y * res + x) * 4 + c] = tile.splatmap[idx + layer];
					}
				}
			}

			// Upload to OpenGL
			GLuint tex;
			glGenTextures(1, &tex);
			glBindTexture(GL_TEXTURE_2D, tex);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (GLsizei) res, (GLsizei) res, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			tile.splatTextures.push_back(tex);

			// Save to PNG for debugging
			std::string filename = "debug/splat/tile_" + tile.name + "_layerGroup_" + std::to_string(i) + ".png";
			stbi_write_png(filename.c_str(), (GLsizei) res, (GLsizei) res, 4, rgba.data(), (GLsizei) res * 4);
		}
	}

	const std::vector<std::unique_ptr<TerrainTile>>& TerrainManager::GetTerrains() const
	{
		return terrains;
	}


} // namespace Engine::Terrain
