#pragma once
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
#include "TerrainTile.h"

typedef unsigned int GLuint;
namespace Engine::Terrain {


	class TerrainManager : public Module {
	  public:
		void Render();

		void                      onInit() override;
		void                      onUpdate(float dt) override;
		void                      onGameStart() override {}
		void                      onShutdown() override;
		[[nodiscard]] std::string name() const override { return "TerrainModule"; };
	};

} // namespace Engine::Terrain
