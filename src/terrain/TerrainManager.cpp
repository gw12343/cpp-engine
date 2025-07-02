#include "TerrainManager.h"
#include "utils/Utils.h"
#include "core/EngineData.h"
#include "glad/glad.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include "rendering/Renderer.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"
#include <stdexcept>

namespace Engine::Terrain {

	void TerrainManager::onInit()
	{
		AssetHandle<Texture> tex1 = GetAssetManager().Load<Texture>("resources/textures/Terrain Grass.png");
		AssetHandle<Texture> tex2 = GetAssetManager().Load<Texture>("resources/textures/Terrain Dirt.png");
		AssetHandle<Texture> tex3 = GetAssetManager().Load<Texture>("resources/textures/Terrain Sand.png");
		AssetHandle<Texture> tex4 = GetAssetManager().Load<Texture>("resources/textures/Terrain Rock.png");
		AssetHandle<Texture> tex5 = GetAssetManager().Load<Texture>("resources/textures/white.png");


		LoadTerrainFile("resources/terrain/TerrainA.bin");
		// LoadTerrainFile("resources/terrain/TerrainB.bin");
		// LoadTerrainFile("resources/terrain/TerrainC.bin");
		// LoadTerrainFile("resources/terrain/TerrainD.bin");
		GenerateMeshes();
		GenerateSplatTextures();
		SetupShaders();


		for (auto& a : GetTerrains()) {
			auto tile = GetAssetManager().Get(a);
			tile->diffuseTextures.push_back(tex1);
			tile->diffuseTextures.push_back(tex2);
			tile->diffuseTextures.push_back(tex3);
			tile->diffuseTextures.push_back(tex4);
			tile->diffuseTextures.push_back(tex5);
		}
	}

	void TerrainManager::onUpdate(float dt)
	{
	}

	void TerrainManager::Render()
	{
		for (auto& ter : terrains) {
			auto tile = GetAssetManager().Get(ter);

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
			for (size_t i = 0; i < tile->diffuseTextures.size(); ++i) {
				GetAssetManager().Get(tile->diffuseTextures[i])->Bind(base + i);
			}

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
		auto tile = GetAssetManager().Load<Terrain::TerrainTile>(path);
		SPDLOG_INFO("LAODED TILE: {}, {}", tile.GetID(), tile.IsValid());
		terrains.push_back(tile);
	}


	void TerrainManager::GenerateMeshes()
	{
		for (auto& ter : terrains) {
			spdlog::info("TERRAIN MESH GEN: {}", ter.GetID());
			GenerateMeshForTile(ter);
		}
	}

	void TerrainManager::SetupShaders()
	{
		for (auto& ter : terrains) {
			auto tile = GetAssetManager().Get(ter);

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
		for (auto& ter : terrains) {
			GenerateSplatTexturesForTile(ter);
		}
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
		auto tile         = GetAssetManager().Get(terrains[0]);
		int  totalLayers  = (int) tile->splatLayerCount;
		int  textureCount = (totalLayers + 3) / 4;

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


	void TerrainManager::GenerateMeshForTile(Engine::AssetHandle<TerrainTile> tileRef)
	{
		auto     tile = GetAssetManager().Get(tileRef);
		uint32_t res  = tile->heightRes;

		std::vector<glm::vec3> positions(res * res);
		std::vector<glm::vec2> uvs(res * res);
		std::vector<glm::vec3> normals(res * res, glm::vec3(0.0f));
		std::vector<float>     vertices;
		std::vector<uint32_t>  indices;

		tile->physicsMesh = JPH::TriangleList();

		auto getHeight = [&](int x, int z) -> float {
			x = std::clamp(x, 0, int(res) - 1);
			z = std::clamp(z, 0, int(res) - 1);
			return tile->heightmap[z * res + x];
		};


		// Step 1: Generate vertex positions and UVs
		for (uint32_t z = 0; z < res; ++z) {
			for (uint32_t x = 0; x < res; ++x) {
				auto u = (float) ((float) x / float(res - 1));
				auto v = (float) ((float) z / float(res - 1));
				auto h = (float) (getHeight((int) x, (int) z));

				glm::vec3 pos(tile->sizeX * u, h * tile->sizeY, tile->sizeZ * v);
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
					tile->physicsMesh.emplace_back(toF3(positions[i0]), toF3(positions[i2]), toF3(positions[i1]));
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
					tile->physicsMesh.emplace_back(toF3(positions[i1]), toF3(positions[i2]), toF3(positions[i3]));
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

		tile->indexCount = indices.size();

		glGenVertexArrays(1, &tile->vao);
		glGenBuffers(1, &tile->vbo);
		glGenBuffers(1, &tile->ebo);

		glBindVertexArray(tile->vao);
		glBindBuffer(GL_ARRAY_BUFFER, tile->vbo);
		glBufferData(GL_ARRAY_BUFFER, (GLsizei) vertices.size() * (GLsizei) sizeof(float), vertices.data(), GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tile->ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizei) indices.size() * (GLsizei) sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

		glEnableVertexAttribArray(0); // position
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*) nullptr);
		glEnableVertexAttribArray(1); // uv
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*) (3 * sizeof(float)));
		glEnableVertexAttribArray(2); // normal
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*) (5 * sizeof(float)));

		glBindVertexArray(0);
	}


	void TerrainManager::GenerateSplatTexturesForTile(Engine::AssetHandle<TerrainTile> tileRef)
	{
		auto tile = GetAssetManager().Get(tileRef);

		uint32_t layerCount = tile->splatLayerCount;
		uint32_t res        = tile->splatRes;
		uint32_t count      = (layerCount + 3) / 4;

		std::filesystem::create_directories("debug/splat");

		for (uint32_t i = 0; i < count; ++i) {
			std::vector<uint8_t> rgba(res * res * 4, 0);

			for (uint32_t y = 0; y < res; ++y) {
				for (uint32_t x = 0; x < res; ++x) {
					int idx = (int) (y * res + x) * (int) layerCount;
					for (int c = 0; c < 4; ++c) {
						uint32_t layer = i * 4 + c;
						if (layer < layerCount) rgba[(y * res + x) * 4 + c] = tile->splatmap[idx + layer];
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
			tile->splatTextures.push_back(tex);

			// Save to PNG for debugging
			std::string filename = "debug/splat/tile_" + tile->name + "_layerGroup_" + std::to_string(i) + ".png";
			stbi_write_png(filename.c_str(), (GLsizei) res, (GLsizei) res, 4, rgba.data(), (GLsizei) res * 4);
		}
	}

	const std::vector<AssetHandle<TerrainTile>>& TerrainManager::GetTerrains() const
	{
		return terrains;
	}


} // namespace Engine::Terrain

#include "assets/AssetManager.inl"