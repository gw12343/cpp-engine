//
// Created by gabe on 7/3/25.
//

#include "TerrainTile.h"
#include "core/EngineData.h"



#include <sstream>
#include "rendering/Renderer.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"
#include "components/impl/TerrainRendererComponent.h"
#include "Jolt/Physics/Collision/Shape/HeightFieldShape.h"
#include <stdexcept>

namespace Engine::Terrain {

	std::string TerrainTile::GenerateGLSLShader() const
	{
		std::ostringstream ss;
		ss << "#version 420 core\n"
              "layout (location = 0) out vec4 gAlbedo;    // RGB = base color, A = alpha\n"
              "layout (location = 1) out vec3 gNormal;    // world-space normal\n"
              "layout (location = 2) out vec4 gMaterial;  // R = specular strength\n"
              "layout (location = 3) out vec3 gEmissive;  // emissive color\n"
              "\n"
              "in VS_OUT {\n"
              "    vec3 FragPos;\n"
              "    vec3 Normal;\n"
              "    vec2 TexCoords;\n"
              "} fs_in;\n"
              "uniform vec2 textureScale;\n\n";





		// Bind terrain texture samplers
		int totalLayers  = static_cast<int>(splatLayerCount);
		int textureCount = (totalLayers + 3) / 4;

		for (int i = 0; i < textureCount; ++i)
			ss << "layout(binding = " << i << ") uniform sampler2D splat" << i << ";\n";
		for (int i = 0; i < totalLayers; ++i)
			ss << "layout(binding = " << (textureCount + i) << ") uniform sampler2D tex" << i << ";\n";





		// Begin main()
		ss << "void main() {\n";
		ss << "  float weights[" << totalLayers << "];\n";
		ss << "  float total = 0.0001;\n";

		for (int i = 0; i < textureCount; ++i) {
			ss << "  vec4 w" << i << " = texture(splat" << i << ", fs_in.TexCoords);\n";
			for (int j = 0; j < 4; ++j) {
				int idx = i * 4 + j;
				if (idx >= totalLayers) break;
				ss << "  weights[" << idx << "] = w" << i << "[" << j << "];\n";
				ss << "  total += weights[" << idx << "];\n";
			}
		}

		ss << "  vec4 baseColor = vec4(0.0);\n";
		for (int i = 0; i < totalLayers; ++i) {
			ss << "  baseColor += texture(tex" << i << ", fs_in.TexCoords * textureScale) * (weights[" << i << "] / total);\n";
		}


        ss << "gAlbedo = baseColor;\n";
        ss << "gNormal = normalize(fs_in.Normal);\n";
        ss << "gMaterial = vec4(0);";
        ss << "gEmissive = vec3(0);";


        ss << "}\n";

		return ss.str();
	}

	std::string TerrainTile::GenerateGLSLVertexShader() const
	{
		std::ostringstream ss;
//		ss << "#version 420\n";
//		ss << "layout(location = 0) in vec3 aPosition;\n";
//		ss << "layout(location = 1) in vec2 aUV;\n";
//		ss << "layout(location = 2) in vec3 aNormal;\n";
//		ss << "uniform mat4 uModel;\n";
//		ss << "uniform mat4 uView;\n";
//		ss << "uniform mat4 uProjection;\n";
//		ss << "out vec2 vUV;\n";
//		ss << "out vec3 vNormal;\n";
//		ss << "out vec3 vWorldPos;\n";
//		ss << "void main() {\n";
//		ss << "  vUV = aUV;\n";
//		ss << "  vec4 worldPos = uModel * vec4(aPosition, 1.0);\n";
//		ss << "  vWorldPos = worldPos.xyz;\n";
//		ss << "  vNormal = mat3(transpose(inverse(uModel))) * aNormal;\n";
//		ss << "  gl_Position = uProjection * uView * worldPos;\n";
//		ss << "}\n";

        ss << "#version 330 core\n"
              "\n"
              "layout (location = 0) in vec3 aPos;\n"
              "layout (location = 1) in vec2 aTexCoord;\n"
              "layout (location = 2) in vec3 aNormal;\n"
              "\n"
              "out VS_OUT {\n"
              "    vec3 FragPos;\n"
              "    vec3 Normal;\n"
              "    vec2 TexCoords;\n"
              "} vs_out;\n"
              "\n"
              "uniform mat4 model;\n"
              "uniform mat4 view;\n"
              "uniform mat4 projection;\n"
              "\n"
              "void main()\n"
              "{\n"
              "    // World position\n"
              "    vs_out.FragPos = vec3(model * vec4(aPos, 1.0));\n"
              "\n"
              "    // Correct normal transform\n"
              "    mat3 normalMatrix = transpose(inverse(mat3(model)));\n"
              "    vec3 N = normalize(normalMatrix * aNormal);\n"
              "    vs_out.Normal = N;\n"
              "    vs_out.TexCoords = aTexCoord;\n"
              "    gl_Position = projection * view * vec4(vs_out.FragPos, 1.0);\n"
              "}\n";

		return ss.str();
	}


	void TerrainTile::SetupShader()
	{
		std::string vertexCode   = GenerateGLSLVertexShader();
		std::string fragmentCode = GenerateGLSLShader();


        //spdlog::info("VERTEX CODE: \n{}\n FRAGMENT CODE: \n{}", vertexCode, fragmentCode);

		terrainShader = std::make_shared<Engine::Shader>();
		bool success  = terrainShader->LoadFromSource(vertexCode, fragmentCode);
		ENGINE_VERIFY(success, "Failed to compile terrain shader");

		spdlog::debug("num of textures: {}", splatTextures.size());
	}

	JPH::Float3 toF3(glm::vec3 v)
	{
		return {v.x, v.y, v.z};
	}


	void TerrainTile::GenerateMesh()
	{
		uint32_t res = heightRes;

		std::vector<glm::vec3> positions(res * res);
		std::vector<glm::vec2> uvs(res * res);
		std::vector<glm::vec3> normals(res * res, glm::vec3(0.0f));
		std::vector<float>     vertices;
		std::vector<uint32_t>  indices;

		auto getHeight = [&](int x, int z) -> float {
			x = std::clamp(x, 0, int(res) - 1);
			z = std::clamp(z, 0, int(res) - 1);
			return heightmap[z * res + x];
		};

		// Step 1: Generate vertex positions and UVs
		for (uint32_t z = 0; z < res; ++z) {
			for (uint32_t x = 0; x < res; ++x) {
				auto u = float(x) / float(res - 1);
				auto v = float(z) / float(res - 1);
				auto h = getHeight((int) x, (int) z);

				glm::vec3 pos(sizeX * u, h * sizeY, sizeZ * v);
				positions[z * res + x] = pos;
				uvs[z * res + x]       = glm::vec2(u, v);
			}
		}

		// Step 2: Generate indices and normals
		for (uint32_t z = 0; z < res - 1; ++z) {
			for (uint32_t x = 0; x < res - 1; ++x) {
				uint32_t i0 = z * res + x;
				uint32_t i1 = i0 + 1;
				uint32_t i2 = i0 + res;
				uint32_t i3 = i2 + 1;

				// Triangle 1
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
				}
				// Triangle 2
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
				}
			}
		}

		// Step 3: Normalize normals and create vertex buffer
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

		indexCount = indices.size();

		glGenVertexArrays(1, &vao);
		glGenBuffers(1, &vbo);
		glGenBuffers(1, &ebo);

		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

		glEnableVertexAttribArray(0); // position
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*) nullptr);
		glEnableVertexAttribArray(1); // uv
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*) (3 * sizeof(float)));
		glEnableVertexAttribArray(2); // normal
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*) (5 * sizeof(float)));

		glBindVertexArray(0);

		// -------- HEIGHTFIELD SHAPE CREATION (no body) --------

		std::vector<uint8_t>       materialIndices(heightRes * heightRes, 0);
		Array<PhysicsMaterialRefC> materials = {new JPH::PhysicsMaterial()};

		const float cellSizeX = sizeX / float(res - 1);
		const float cellSizeZ = sizeZ / float(res - 1);

		JPH::Vec3 terrainOffset = JPH::Vec3(0, 0, 0);
		JPH::Vec3 terrainScale  = JPH::Vec3(cellSizeX, sizeY, cellSizeZ);

		JPH::HeightFieldShapeSettings settings(heightmap.data(), terrainOffset, terrainScale, res, materialIndices.data(), materials);
		settings.mBlockSize     = 1 << 3;
		settings.mBitsPerSample = 8;

		auto shape_result = settings.Create();
		if (shape_result.HasError()) {
			spdlog::error("Failed to create HeightFieldShape: {}", shape_result.GetError().c_str());
			return;
		}

		// Store for later body creation
		heightfieldShape = shape_result.Get();
	}

	void TerrainTile::GenerateSplatTextures()
	{
		uint32_t layerCount = splatLayerCount;
		uint32_t res        = splatRes;
		uint32_t count      = (layerCount + 3) / 4;

		std::filesystem::create_directories("debug/splat");

		for (uint32_t i = 0; i < count; ++i) {
			std::vector<uint8_t> rgba(res * res * 4, 0);

			for (uint32_t y = 0; y < res; ++y) {
				for (uint32_t x = 0; x < res; ++x) {
					int idx = (int) (y * res + x) * (int) layerCount;
					for (int c = 0; c < 4; ++c) {
						uint32_t layer = i * 4 + c;
						if (layer < layerCount) rgba[(y * res + x) * 4 + c] = splatmap[idx + layer];
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
			splatTextures.push_back(tex);

			// Save to PNG for debugging
			std::string filename = "debug/splat/tile_" + name + "_layerGroup_" + std::to_string(i) + ".png";
			stbi_write_png(filename.c_str(), (GLsizei) res, (GLsizei) res, 4, rgba.data(), (GLsizei) res * 4);
		}
	}

} // namespace Engine::Terrain