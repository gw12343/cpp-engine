#include "TerrainManager.h"
#include "core/EngineData.h"
#include "glad/glad.h"
#include "rendering/Renderer.h"

#include "stb/stb_image_write.h"
#include "components/impl/EntityMetadataComponent.h"
#include "components/impl/TransformComponent.h"
#include "animation/TerrainRendererComponent.h"

namespace Engine::Terrain {

	void TerrainManager::onInit()
	{
	}

	void TerrainManager::onUpdate(float dt)
	{
		for (auto& terrPairs : GetAssetManager().GetStorage<TerrainTile>().assets) {
			auto& tile = terrPairs.second;

			// todo move!!
			AssetHandle<Texture> tex1 = GetAssetManager().Load<Texture>("resources/textures/Terrain Grass.png");
			AssetHandle<Texture> tex2 = GetAssetManager().Load<Texture>("resources/textures/Terrain Dirt.png");
			AssetHandle<Texture> tex3 = GetAssetManager().Load<Texture>("resources/textures/Terrain Sand.png");
			AssetHandle<Texture> tex4 = GetAssetManager().Load<Texture>("resources/textures/Terrain Rock.png");
			AssetHandle<Texture> tex5 = GetAssetManager().Load<Texture>("resources/textures/white.png");


			tile->diffuseTextures.clear();
			tile->diffuseTextures.push_back(tex1);
			tile->diffuseTextures.push_back(tex2);
			tile->diffuseTextures.push_back(tex3);
			tile->diffuseTextures.push_back(tex4);
			tile->diffuseTextures.push_back(tex5);
		}
	}

	void TerrainManager::Render()
	{
		auto view = GetRegistry().view<Components::EntityMetadata, Components::Transform, Components::TerrainRenderer>();
		for (auto [entity, metadata, transform, renderer] : view.each()) {
			if (!renderer.visible) continue;
			if (!renderer.terrainTile.IsValid()) continue;

			auto      tile             = GetAssetManager().Get(renderer.terrainTile);
			glm::mat4 viewM            = GetCamera().GetViewMatrix();
			glm::mat4 terrainTransform = transform.GetMatrix();

			GLuint shadowSlot = tile->splatTextures.size() + tile->diffuseTextures.size();
			GetRenderer().GetShadowRenderer()->UploadShadowMatrices(*tile->terrainShader, viewM, shadowSlot);
			tile->terrainShader->Bind();
			tile->terrainShader->SetMat4("uModel", &terrainTransform);
			tile->terrainShader->SetBool("debugShadows", false);

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
} // namespace Engine::Terrain

#include "assets/AssetManager.inl"