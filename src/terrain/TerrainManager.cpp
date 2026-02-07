#include <tracy/Tracy.hpp>
#include "TerrainManager.h"
#include "core/EngineData.h"
#include "glad/glad.h"
#include "rendering/Renderer.h"

#include "stb/stb_image_write.h"
#include "components/impl/EntityMetadataComponent.h"
#include "components/impl/TransformComponent.h"
#include "components/impl/TerrainRendererComponent.h"
#include "utils/Utils.h"

namespace Engine::Terrain {

	void TerrainManager::onInit()
	{
        for (auto& terrPairs : GetAssetManager().GetStorage<TerrainTile>().guidToAsset) {
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

	void TerrainManager::onUpdate(float dt)
	{
		ZoneScoped;

	}


//        Shader& gbufferShader = GetGBufferShader();
//
//        gbufferShader.Bind();
//
//        // View / projection matricies
//
//        glm::mat4 V = GetCamera().GetViewMatrix();
//        gbufferShader.SetMat4("view", &V);
//        glm::mat4 proj = GetCamera().GetProjectionMatrix();
//        gbufferShader.SetMat4("projection", &proj);
//
//        ENGINE_GLCheckError();
//
//        auto view = GetCurrentSceneRegistry().view<
//                Engine::Components::EntityMetadata,
//                Engine::Components::Transform,
//                Engine::Components::ModelRenderer
//        >();
//
//        for (auto [entity, metadata, transform, renderer] : view.each()) {
//            if (!renderer.visible)
//                continue;
//
//            renderer.Draw(gbufferShader, transform, true);
//        }

	void TerrainManager::RenderGBuffer()
	{
        ZoneScopedN("Render terrain GBuffer");

        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        glDisable(GL_BLEND);

        ENGINE_GLCheckError();



		auto view = GetCurrentSceneRegistry().view<Components::EntityMetadata, Components::Transform, Components::TerrainRenderer>();
		for (auto [entity, metadata, transform, renderer] : view.each()) {

            if (!renderer.visible) continue;
            if (!renderer.terrainTile.IsValid()) continue;
			auto tile = GetAssetManager().Get(renderer.terrainTile);
			if (tile == nullptr) continue;


			glm::mat4 viewM            = GetCamera().GetViewMatrix();
			glm::mat4 terrainTransform = transform.GetWorldMatrix();


            auto gbufferShader = tile->terrainShader;

			gbufferShader->Bind();
			gbufferShader->SetMat4("model", &terrainTransform);


            glm::mat4 V = GetCamera().GetViewMatrix();
            gbufferShader->SetMat4("view", &V);
            glm::mat4 proj = GetCamera().GetProjectionMatrix();
            gbufferShader->SetMat4("projection", &proj);


            gbufferShader->SetVec2("textureScale", glm::vec2(100.0, 100.0));

            ENGINE_GLCheckError();


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
        glEnable(GL_BLEND);
	}

    void TerrainManager::Render()
	{
		auto view = GetCurrentSceneRegistry().view<Components::EntityMetadata, Components::Transform, Components::TerrainRenderer>();
		for (auto [entity, metadata, transform, renderer] : view.each()) {
			if (!renderer.visible) continue;
			if (!renderer.terrainTile.IsValid()) continue;

			auto tile = GetAssetManager().Get(renderer.terrainTile);
			if (tile == NULL) continue; // todo warn
			glm::mat4 viewM            = GetCamera().GetViewMatrix();
			glm::mat4 terrainTransform = transform.GetWorldMatrix();

			GLuint shadowSlot = tile->splatTextures.size() + tile->diffuseTextures.size();
			GetRenderer().GetShadowRenderer()->UploadShadowMatrices(*tile->terrainShader, viewM, static_cast<int>(shadowSlot));
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