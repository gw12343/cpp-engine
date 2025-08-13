//
// Created by gabe on 7/3/25.
//

#ifndef CPP_ENGINE_TERRAINTILE_H
#define CPP_ENGINE_TERRAINTILE_H

#include "Camera.h"
#include "core/Window.h"
#include "rendering/Shader.h"
#include "rendering/Texture.h"

#include "Jolt/Jolt.h"
#include "Jolt/Geometry/Triangle.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "assets/AssetManager.h"

namespace Engine::Terrain {

	struct TreeInstance {
		float    x, y, z;
		float    scale;
		uint32_t prefabIndex;
	};

	class TerrainTile {
	  public:
		void GenerateMesh();
		void GenerateSplatTextures();
		void SetupShader();

		std::string               name;
		uint32_t                  heightRes;
		uint32_t                  splatRes;
		uint32_t                  splatLayerCount;
		float                     sizeX, sizeY, sizeZ;
		std::vector<float>        heightmap;
		std::vector<uint8_t>      splatmap;
		std::vector<TreeInstance> trees;
		JPH::Ref<JPH::Shape>      heightfieldShape;

		std::shared_ptr<Engine::Shader> terrainShader;
		float                           posX;
		float                           posY;
		float                           posZ;

		// Runtime-generated OpenGL assets
		GLuint                                    vao        = 0;
		GLuint                                    vbo        = 0;
		GLuint                                    ebo        = 0;
		GLuint                                    indexCount = 0;
		std::vector<GLuint>                       splatTextures;   // One RGBA texture per 4 layers
		std::vector<AssetHandle<Engine::Texture>> diffuseTextures; // One RGBA texture per 4 layers
	  private:
		[[nodiscard]] std::string GenerateGLSLShader() const;
		[[nodiscard]] std::string GenerateGLSLVertexShader() const;
	};

} // namespace Engine::Terrain

#endif // CPP_ENGINE_TERRAINTILE_H
