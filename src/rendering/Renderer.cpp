#include "Renderer.h"


#include "assets/impl/ModelLoader.h"

#include "core/EngineData.h"
#include "core/Input.h"
#include "terrain/TerrainManager.h"
#include "animation/AnimationManager.h"
#include "rendering/particles/ParticleManager.h"
#include "components/impl/EntityMetadataComponent.h"
#include "components/impl/TransformComponent.h"
#include "components/impl/ModelRendererComponent.h"
#include "components/impl/SkinnedMeshComponent.h"
#include "components/impl/GizmoComponent.h"
#include <spdlog/spdlog.h>

#include <RmlUi/Core.h>
#include "rendering/ui/RmlUiBackend.h"
#include "rendering/ui/GameUIManager.h"
#include "scripting/ScriptManager.h"

#include "components/impl/AnimationComponent.h"
#include "Texture.h"

#include <random>

#define RENDER_STEP(name) ZoneScopedN(name); DebugGroup group(name);

namespace Engine {
    std::vector<glm::vec3> ssaoKernel;


    void Renderer::onInit() {
        ZoneScopedN("Initialize Renderer");

        m_shadowRenderer = std::make_shared<ShadowMapRenderer>();
        m_bloomRenderer = std::make_shared<BloomRenderer>();
        m_text3DRenderer = std::make_unique<Text3DRenderer>();

        {
            ZoneScopedN("Initialize BloomRenderer");
            m_bloomRenderer->Initialize();
        }
        {
            ZoneScopedN("Load Shaders");
            ReloadShaders();
        }
        {
            ZoneScopedN("Initialize Text3DRenderer");
            m_text3DRenderer->Initialize();
        }
        {
            ZoneScopedN("Load Skybox");
            m_skybox = std::make_unique<Skybox>();

            const std::string p = "resources/textures/output2.hdr";
            if (!m_skybox->LoadFromFile(p)) {
                log->error("Failed to load skybox");
                return;
            }
        }

        // Enable depth testing so closer fragments obscure farther ones
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        {
            ZoneScopedN("Initialize ShadowMapRenderer");
            m_shadowRenderer->Initialize();
        }

        {
            ZoneScopedN("Create fullscreen quad");
            InitFullscreenQuad();
        }

        {
            ZoneScopedN("Generate SSAO kernel");
            // Generate ssao kernel
            ssaoKernel.clear();
            ssaoKernel.reserve(32);

            // Fixed seed so kernel is stable between runs (less shimmer).
            std::mt19937 rng(1337u);
            std::uniform_real_distribution<float> dist01(0.0f, 1.0f);
            for (int i = 0; i < 32; i++) {
                // Hemisphere sample (z >= 0 in tangent space)
                glm::vec3 sample = glm::normalize(glm::vec3(
                        dist01(rng) * 2.0f - 1.0f,
                        dist01(rng) * 2.0f - 1.0f,
                        dist01(rng) // 0..1 hemisphere
                ));

                // Accelerate distribution toward the origin
                float scale = (float) i / 32.0f;
                scale = glm::mix(0.1f, 1.0f, scale * scale);

                sample *= scale;
                ssaoKernel.push_back(sample);
            }
        }
    }

    void Renderer::InitFullscreenQuad() {
        if (quadVAO != 0) return;

        float quadVertices[] = {
                // positions   // tex coords
                -1.0f, -1.0f, 0.0f, 0.0f,
                1.0f, -1.0f, 1.0f, 0.0f,
                1.0f, 1.0f, 1.0f, 1.0f,

                -1.0f, -1.0f, 0.0f, 0.0f,
                1.0f, 1.0f, 1.0f, 1.0f,
                -1.0f, 1.0f, 0.0f, 1.0f
        };

        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);

        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *) 0);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                              (void *) (2 * sizeof(float)));

        glBindVertexArray(0);
    }

    void Renderer::PreRender() {
        // Clear the screen
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Bind our shader
        m_shader.Bind();

        // Set up view and projection matrices
        glm::mat4 view = GetCamera().GetViewMatrix();
        glm::mat4 projection = GetCamera().GetProjectionMatrix();

        m_shader.SetMat4("view", &view);
        m_shader.SetMat4("projection", &projection);
    }

    void Renderer::PostRender() {
        GetWindow().SwapBuffers();
    }

    void Renderer::onShutdown() {
        Texture::CleanAllTextures();
        Rendering::Mesh::CleanAllMeshes();

        m_skybox.reset();
        if (m_text3DRenderer) {
            m_text3DRenderer->Shutdown();
            m_text3DRenderer.reset();
        }
        m_bloomRenderer.reset();
        m_shadowRenderer.reset();

        m_shader.Destroy();
        m_mousePickingShader.Destroy();
        m_modelPreviewShader.Destroy();
        m_materialPreviewShader.Destroy();
        m_terrainShader.Destroy();
        m_gbufferShader.Destroy();
        m_lightingShader.Destroy();
        m_ssaoShader.Destroy();
        m_ssaoBlurShader.Destroy();

        if (quadVAO != 0) {
            glDeleteVertexArrays(1, &quadVAO);
            quadVAO = 0;
        }
        if (quadVBO != 0) {
            glDeleteBuffers(1, &quadVBO);
            quadVBO = 0;
        }
    }

    void Renderer::RenderBloomPass() {
        RENDER_STEP("Render Bloom Pass");
        m_bloomRenderer->RenderBloom();
    }

    void Renderer::RenderLightingPass() {

        RENDER_STEP("Deferred Lighting");

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);

        m_lightingShader.Bind();

        // G-buffer textures
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, GetWindow().GetGBuffer()->GetDepth());
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, GetWindow().GetGBuffer()->GetNormal());
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, GetWindow().GetGBuffer()->GetAlbedo());
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, GetWindow().GetGBuffer()->GetMaterial());
        m_skybox->m_texture->Bind(4);
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, GetWindow().GetSSAOBuffer()->blurTex);

        glActiveTexture(GL_TEXTURE7);
        glBindTexture(GL_TEXTURE_2D, GetWindow().GetGBuffer()->GetEmissive());

        glm::mat4 V = GetCamera().GetViewMatrix();
        glm::mat4 viewInv = glm::inverse(V);
        m_lightingShader.SetMat4("invView", &viewInv);
        glm::mat4 projInv = glm::inverse(GetCamera().GetProjectionMatrix());
        m_lightingShader.SetMat4("invProjection", &projInv);


        m_lightingShader.SetInt("gDepth", 0);
        m_lightingShader.SetInt("gNormal", 1);
        m_lightingShader.SetInt("gAlbedo", 2);
        m_lightingShader.SetInt("gMaterial", 3);
        m_lightingShader.SetInt("skybox", 4);
        m_lightingShader.SetInt("ssaoBlurTex", 5);

        m_lightingShader.SetInt("gEmissive", 7);
        m_lightingShader.SetInt("bloomTex", 8);

        // Camera + light uniforms
        m_shadowRenderer->UploadShadowMatrices(m_lightingShader, V, 6);



        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        glEnable(GL_DEPTH_TEST);
    }


    void Renderer::onUpdate(float dt) {
        ZoneScopedN("Render");

        PreRender();

        // CPU-skin all characters once; shadow / GBuffer / pick reuse the cache.
        GetAnimationManager().PrepareSkinnedMeshes();

        // Shadows
        RenderShadowMaps();

        {
            RENDER_STEP("Deferred GBuffer Pass");

            GetWindow().GetGBuffer()->Bind();


            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


            RenderEntitiesGBuffer();
            {
                RENDER_STEP("Deferred GBuffer Animations Pass");
                GetAnimationManager().Render();
            }
            GetTerrainManager().RenderGBuffer();

            GetWindow().GetGBuffer()->Unbind();
        }


        // SSAO
        RenderSSAO();
        RenderSSAOBlur();




        // Draw to the lighting framebuffer
        Engine::Window::GetFramebuffer(Window::FramebufferID::LIGHTING)->Bind();

        // Do lighting calculations with GBuffer
        RenderLightingPass();
        Framebuffer::Unbind();


        RenderBloomPass();

#ifndef GAME_BUILD
        // Draw to the game viewport framebuffer
        Engine::Window::GetFramebuffer(Window::FramebufferID::GAME_OUT)->Bind();
#else
        // Just draw to the screen
        Engine::Framebuffer::Unbind();
#endif

#ifndef GAME_BUILD
        {
            if (GetState() == EDITOR || GetState() == PAUSED) {
                {
                    ZoneScopedN("Render Gizmos");
                    DebugGroup group("Render Gizmos");

                    RenderGizmos(false);
                }

            }
        }

        {
            RENDER_STEP("Animations Debug Skeleton Pass");
            GetAnimationManager().RenderDebug();
        }
#endif

        {
            ZoneScopedN("Render Particles");
            DebugGroup group("Render Particles");
            GetParticleManager().Render();
        }

        {
            RENDER_STEP("Render Text3D");
            RenderText3D();
        }

        {
            ZoneScopedN("Render RmlUi");
            DebugGroup group("Render RmlUi");
            // Render RmlUi into the framebuffer
            GetGameUIManager().Render();
        }

#ifndef GAME_BUILD
        // Mouse picking FBO is only sampled on click (SceneViewWindow). Still rebuild
        // every editor frame so the click reads current geometry — but skip entirely
        // while playing, and when the cursor is outside the game viewport (no selection).
        if (GetState() != PLAYING && GetInput().IsMousePositionInViewport()) {
            ZoneScopedN("Render Mouse Picking");
            DebugGroup group("Render Mouse Picking");
            Engine::Window::GetFramebuffer(Window::FramebufferID::MOUSE_PICKING)->Bind();
            RenderEntitiesMousePicking();
        }
        GetScriptManager().EditorScriptUpdate(dt);
#endif


        Engine::Framebuffer::Unbind();

        // Kick the driver so the deferred/shadow/lighting work can execute on the GPU
        // while the CPU finalizes ImGui draw lists in PostRender (reduces SwapBuffers wait).
        glFlush();

        {
            ZoneScopedN("Post Render");
            PostRender();
        }
    }





    void Renderer::RenderSSAO() {
        RENDER_STEP("Render SSAO Pass");
        auto ssaoBuf = GetWindow().GetSSAOBuffer();
        ssaoBuf->BindSSAO();

        glClearColor(1.f, 1.f, 1.f, 1.f); // unoccluded default
        glClear(GL_COLOR_BUFFER_BIT);
        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        glDisable(GL_BLEND);

        m_ssaoShader.Bind();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, GetWindow().GetGBuffer()->GetDepth());
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, GetWindow().GetGBuffer()->GetNormal());
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, ssaoBuf->noiseTex);

        glm::mat4 V       = GetCamera().GetViewMatrix();
        glm::mat4 proj    = GetCamera().GetProjectionMatrix();
        glm::mat4 projInv = glm::inverse(proj);
        m_ssaoShader.SetMat4("view", &V);
        m_ssaoShader.SetMat4("projection", &proj);
        m_ssaoShader.SetMat4("invProjection", &projInv);

        m_ssaoShader.SetInt("gDepth", 0);
        m_ssaoShader.SetInt("gNormal", 1);
        m_ssaoShader.SetInt("noiseTex", 2);

        int loc = glGetUniformLocation(m_ssaoShader.GetProgramID(), "samples");
        glUniform3fv(loc, 32, &ssaoKernel[0].x);

        // Noise tile scale must use SSAO FBO size, not OS window size.
        m_ssaoShader.SetVec2("screenSize", glm::vec2(static_cast<float>(ssaoBuf->width), static_cast<float>(ssaoBuf->height)));

        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        GetWindow().SetFullViewport();
    }

    void Renderer::RenderSSAOBlur() {
        RENDER_STEP("Render SSAO Blur Pass");
        auto ssaoBuf = GetWindow().GetSSAOBuffer();
        ssaoBuf->BindBlur();

        glClearColor(1.f, 1.f, 1.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);

        m_ssaoBlurShader.Bind();
        m_ssaoBlurShader.SetInt("ssaoInput", 0);
        m_ssaoBlurShader.SetInt("gDepth", 1);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, ssaoBuf->ssaoTex);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, GetWindow().GetGBuffer()->GetDepth());

        m_ssaoBlurShader.SetVec2("screenSize", glm::vec2(static_cast<float>(ssaoBuf->width), static_cast<float>(ssaoBuf->height)));

        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        GetWindow().SetFullViewport();
    }

    void Renderer::RenderEntitiesGBuffer() {
        ZoneScopedN("Render Entities GBuffer");

        glDisable(GL_CULL_FACE);          // match forward for now
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);

        ENGINE_GLCheckError();

        Shader &gbufferShader = GetGBufferShader();

        gbufferShader.Bind();

        // View / projection matricies

        glm::mat4 V = GetCamera().GetViewMatrix();
        gbufferShader.SetMat4("view", &V);
        glm::mat4 proj = GetCamera().GetProjectionMatrix();
        gbufferShader.SetMat4("projection", &proj);

        ENGINE_GLCheckError();

        auto view = GetCurrentSceneRegistry().view<
                Engine::Components::EntityMetadata,
                Engine::Components::Transform,
                Engine::Components::ModelRenderer
        >();

        for (auto [entity, metadata, transform, renderer]: view.each()) {
            if (!renderer.visible)
                continue;

            renderer.Draw(gbufferShader, transform, true);
        }

        //TODO add unbind??
        //gbufferShader.Unbind();
    }

    glm::vec3 EncodeEntityID(entt::entity entityID) {
        auto id = static_cast<uint32_t>(entityID);
        float r = (float) (id & 0xFF) / 255.0f;
        float g = (float) ((id >> 8) & 0xFF) / 255.0f;
        float b = (float) ((id >> 16) & 0xFF) / 255.0f;
        return {r, g, b};
    }

    void Renderer::RenderEntitiesMousePicking() {
        ZoneScopedN("RenderEntitiesMousePicking");
        glDisable(GL_CULL_FACE);

        {
            ZoneScopedN("Mouse Picking Clear + Shader Setup");
            glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            GetMousePickingShader().Bind();

            ENGINE_GLCheckError();
            glm::mat4 V = GetCamera().GetViewMatrix();
            GetMousePickingShader().SetMat4("view", &V);
            glm::mat4 proj = GetCamera().GetProjectionMatrix();
            GetMousePickingShader().SetMat4("projection", &proj);
            ENGINE_GLCheckError();
        }

        {
            ZoneScopedN("Model Renderer Mouse Picking");
            auto view = GetCurrentSceneRegistry().view<Engine::Components::EntityMetadata, Engine::Components::Transform, Engine::Components::ModelRenderer>();
            for (auto [entity, metadata, transform, renderer]: view.each()) {
                glm::vec3 encodedColor = EncodeEntityID(entity);
                GetMousePickingShader().SetVec3("entityIDColor", encodedColor);
                if (!renderer.visible) continue;
                renderer.Draw(GetMousePickingShader(), transform, false);
            }
        }

        {
            ZoneScopedN("Skinned Mesh Mouse Picking");
            GetAnimationManager().PrepareSkinnedMeshes();
            auto view = GetCurrentSceneRegistry().view<Components::SkinnedMeshComponent, Components::Transform>();
            for (auto entity: view) {
                ZoneScopedN("MousePick Skinned Entity");
                Entity e(entity, GetCurrentScene());
                auto &skinned = e.GetComponent<Components::SkinnedMeshComponent>();
                if (!skinned.meshes || skinned.skin_frame_cache.empty()) {
                    continue;
                }
                if (!e.HasComponent<Components::AnimationComponent>()) {
                    continue;
                }
                const ozz::math::Float4x4 transform = FromMatrix(
                        e.GetComponent<Components::Transform>().GetWorldMatrix());

                glm::vec3 encodedColor = EncodeEntityID(entity);

                for (size_t mi = 0; mi < skinned.meshes->size(); ++mi) {
                    const auto& mesh  = (*skinned.meshes)[mi];
                    const auto& cache = skinned.skin_frame_cache[mi];
                    if (!cache.valid) continue;
                    GetAnimationManager().renderer_->DrawSkinnedMeshMousePickingCached(encodedColor, cache, mesh, transform);
                }
            }
        }
        {
            ZoneScopedN("Gizmo Mouse Picking");
            RenderGizmos(true);
        }

        {
            ZoneScopedN("Text3D Mouse Picking");
            if (m_text3DRenderer) {
                m_text3DRenderer->RenderMousePicking();
            }
        }
    }

    void Renderer::RenderShadowMaps() {
        RENDER_STEP("Render Shadow Maps");
        m_shadowRenderer->RenderShadowMaps();
    }

    void Renderer::RenderGizmos(bool mousePicking) {
        auto view = GetCurrentSceneRegistry().view<Engine::Components::EntityMetadata, Engine::Components::Transform, Engine::Components::GizmoComponent>();
        for (auto [entity, metadata, transform, gizmo]: view.each()) {
            const ozz::math::Float4x4 t = FromMatrix(transform.GetWorldMatrix());
            // Draw gizmo

            Color color = kBlack;
            if (mousePicking) {
                glm::vec3 encodedColor = EncodeEntityID(entity);
                color.r = encodedColor.r;
                color.g = encodedColor.g;
                color.b = encodedColor.b;
            } else {
                color.r = gizmo.color.r;
                color.g = gizmo.color.g;
                color.b = gizmo.color.b;
            }
            GetAnimationManager().renderer_->DrawSphereIm(gizmo.radius, t, color);
        }
    }

    void Renderer::ReloadShaders() {
        log->info("Reloading shaders...");
        if (!m_shader.LoadFromFiles("resources/shaders/vert.glsl", "resources/shaders/frag.glsl", std::nullopt)) {
            log->error("Failed to load default shader");
        }

        if (!m_mousePickingShader.LoadFromFiles("resources/shaders/picking.vert", "resources/shaders/picking.frag",
                                                std::nullopt)) {
            log->error("Failed to load mouse picking shader");
        }

        // Load model preview shader
        if (!m_modelPreviewShader.LoadFromFiles("resources/shaders/preview_vert.glsl",
                                                "resources/shaders/preview_frag.glsl", std::nullopt)) {
            log->error("Failed to load model preview shader");
        }

        // Load material preview shader
        if (!m_materialPreviewShader.LoadFromFiles("resources/shaders/material_preview_vert.glsl",
                                                   "resources/shaders/material_preview_frag.glsl", std::nullopt)) {
            log->error("Failed to load material preview shader");
        }

        // Load GBuffer shader
        if (!m_gbufferShader.LoadFromFiles("resources/shaders/gbuffer_vert.glsl", "resources/shaders/gbuffer_frag.glsl",
                                           std::nullopt)) {
            log->error("Failed to load gbuffer shader");
            return;
        }

        // Load lighting shader
        if (!m_lightingShader.LoadFromFiles("resources/shaders/lighting_vert.glsl",
                                            "resources/shaders/lighting_frag.glsl", std::nullopt)) {
            log->error("Failed to load lighting shader");
            return;
        }

        if (!m_ssaoShader.LoadFromFiles("resources/shaders/ssao/ssao_vert.glsl",
                                        "resources/shaders/ssao/ssao_frag.glsl", std::nullopt)) {
            log->error("Failed to load ssao shader");
            return;
        }

        if (!m_ssaoBlurShader.LoadFromFiles("resources/shaders/ssao/ssao_vert.glsl",
                                            "resources/shaders/ssao/ssao_blur_frag.glsl", std::nullopt)) {
            log->error("Failed to load ssao blur shader");
            return;
        }

        m_bloomRenderer->ReloadShaders();

        if (m_text3DRenderer) {
            m_text3DRenderer->ReloadShaders();
        }
    }

    void Renderer::RenderText3D() {
        if (m_text3DRenderer) {
            m_text3DRenderer->Render();
        }
    }

    std::shared_ptr<ShadowMapRenderer> Renderer::GetShadowRenderer() {
        return m_shadowRenderer;
    }



} // namespace Engine