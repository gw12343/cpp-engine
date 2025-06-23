#pragma once

#include "Camera.h"
#include "core/Window.h"
#include "rendering/Shader.h"
#include "rendering/Texture.h"

#include <Jolt/Jolt.h>
#include <Jolt/Geometry/Triangle.h>
#include <glad/glad.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>


namespace Engine::Terrain {

	struct TreeInstance {
		float    x, y, z;
		float    scale;
		uint32_t prefabIndex;
	};

	struct TerrainTile {
		std::string                     name;
		uint32_t                        heightRes;
		uint32_t                        splatRes;
		uint32_t                        splatLayerCount;
		float                           sizeX, sizeY, sizeZ;
		std::vector<float>              heightmap;
		std::vector<uint8_t>            splatmap;
		std::vector<TreeInstance>       trees;
		JPH::TriangleList               physicsMesh;
		std::shared_ptr<Engine::Shader> terrainShader;
		float                           posX;
		float                           posY;
		float                           posZ;


		// Runtime-generated OpenGL assets
		GLuint                                        vao        = 0;
		GLuint                                        vbo        = 0;
		GLuint                                        ebo        = 0;
		GLuint                                        indexCount = 0;
		std::vector<GLuint>                           splatTextures;   // One RGBA texture per 4 layers
		std::vector<std::shared_ptr<Engine::Texture>> diffuseTextures; // One RGBA texture per 4 layers
	};

	class TerrainManager : public Module {
	  public:
		void LoadTerrainFile(const std::string& path);
		void GenerateMeshes();
		void GenerateSplatTextures();

		[[nodiscard]] const std::vector<std::unique_ptr<TerrainTile>>& GetTerrains() const;

		void Render();

		void SetupShaders();


		void        onInit() override;
		void        onUpdate(float dt) override;
		void        onShutdown() override;
		std::string name() const override { return "TerrainModule"; };

	  private:
		[[nodiscard]] std::string GenerateGLSLShader() const;
		[[nodiscard]] std::string GenerateGLSLVertexShader() const;

		std::vector<std::unique_ptr<TerrainTile>> terrains;

		void GenerateMeshForTile(TerrainTile& tile);
		void GenerateSplatTexturesForTile(TerrainTile& tile);
	};

} // namespace Engine::Terrain
