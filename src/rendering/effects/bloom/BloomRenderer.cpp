//
// Created by Gabe on 2/19/2026.
//

#include "BloomRenderer.h"
#include "core/EngineData.h"
#include "rendering/Renderer.h"



namespace Engine {


    void BloomRenderer::Initialize() {
        m_downSampleShader = std::make_shared<Shader>();
        m_upSampleShader = std::make_shared<Shader>();
        m_combineShader = std::make_shared<Shader>();
        ReloadShaders();
        const auto& window = GetWindow();
        ResizeFramebuffers(window.GetWidth(), window.GetHeight());
    }


    void BloomRenderer::ResizeFramebuffers(int render_width, int render_height) {


        m_bloomMips.clear();

        // Start with full resolution and downscale by powers of two
        glm::ivec2 mipSize(render_width, render_height);

        const int MAX_MIPS = 6; // or until size < 16
        for (int i = 0; i < MAX_MIPS; ++i)
        {
            BloomMip mip;
            mip.size = mipSize;

            // Resize the framebuffer for this mip
            mip.fb.ResizeBloomMip(mipSize.x, mipSize.y);

            m_bloomMips.push_back(mip);

            // Half the resolution for the next mip
            mipSize.x = std::max(1, mipSize.x / 2);
            mipSize.y = std::max(1, mipSize.y / 2);

            // Stop if we reach tiny textures
            if (mipSize.x <= 16 || mipSize.y <= 16)
                break;
        }
    }

    void BloomRenderer::ReloadShaders() {
        GetRenderer().log->info("Reloading bloom shaders...");

        std::string vertexShader = "resources/shaders/bloom/bloom_vert.glsl";
        if (!m_downSampleShader->LoadFromFiles(vertexShader, "resources/shaders/bloom/downsample_frag.glsl", std::nullopt)) {
            GetRenderer().log->error("Failed to load downsample shader");
        }

        if (!m_upSampleShader->LoadFromFiles(vertexShader, "resources/shaders/bloom/upsample_frag.glsl", std::nullopt)) {
            GetRenderer().log->error("Failed to load upsample shader");
        }

        if (!m_combineShader->LoadFromFiles(vertexShader, "resources/shaders/bloom/combine_frag.glsl", std::nullopt)) {
            GetRenderer().log->error("Failed to load combine shader");
        }
    }


    void BloomRenderer::RenderBloom() {
        ENGINE_GLCheckError();
        DownsampleChain();
        UpsampleChain();
        CombineScene();
    }


    void BloomRenderer::DownsampleChain() {
        GLuint hdrSceneTex = Window::GetFramebuffer(Window::FramebufferID::LIGHTING)->texture;

        m_downSampleShader->Bind();

        for (int i = 0; i < GetRenderSettings()->BLOOM_MIPS; i++) {
            glActiveTexture(GL_TEXTURE0);

            if (i == 0){
                m_downSampleShader->SetInt("applyThreshold", 1);
                glBindTexture(GL_TEXTURE_2D, hdrSceneTex);
            }
            else{
                m_downSampleShader->SetInt("applyThreshold", 0);
                glBindTexture(GL_TEXTURE_2D, m_bloomMips[i - 1].fb.texture);
            }

            m_downSampleShader->SetInt("srcTex", 0);

            // Pass the size of the source texture
            glm::vec2 srcSize = (i == 0) ?
                                glm::vec2(GetWindow().GetWidth(),
                                          GetWindow().GetHeight())
                                         : m_bloomMips[i - 1].size;
            m_downSampleShader->SetVec2("srcTexSize", srcSize);

            m_bloomMips[i].fb.Bind();
            glViewport(0, 0, (GLsizei)m_bloomMips[i].size.x, (GLsizei)m_bloomMips[i].size.y);
            glClear(GL_COLOR_BUFFER_BIT);

            m_downSampleShader->SetFloat("threshold", GetRenderSettings()->bloom_threshold);
            m_downSampleShader->SetFloat("knee", GetRenderSettings()->bloom_knee);

            glBindVertexArray(GetRenderer().quadVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glBindVertexArray(0);
            Framebuffer::Unbind();
        }
    }

    void BloomRenderer::UpsampleChain() {
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
        m_upSampleShader->Bind();

        for (int i = (GLsizei)GetRenderSettings()->BLOOM_MIPS - 1; i > 0; i--)
        {
            m_bloomMips[i - 1].fb.Bind();
            glViewport(0, 0, (GLsizei)m_bloomMips[i-1].size.x, (GLsizei)m_bloomMips[i-1].size.y);


            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, m_bloomMips[i].fb.texture);
            m_upSampleShader->SetInt("lowResTex", 0);


            glBindVertexArray(GetRenderer().quadVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glBindVertexArray(0);
        }
        glDisable(GL_BLEND);
    }


    void BloomRenderer::CombineScene()
    {
        GLuint hdrSceneTex = Window::GetFramebuffer(Window::FramebufferID::LIGHTING)->texture;

#ifndef GAME_BUILD
        // Draw to the game viewport framebuffer
        Engine::Window::GetFramebuffer(Window::FramebufferID::GAME_OUT)->Bind();
#else
        // Just draw to the screen
        Engine::Framebuffer::Unbind();
#endif

        glDisable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);

        m_combineShader->Bind();

        m_combineShader->SetFloat("bloomStrength", 0.5f);

        // Scene texture (HDR lighting result)
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, hdrSceneTex);
        m_combineShader->SetInt("sceneTex", 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_bloomMips[0].fb.texture);
        m_combineShader->SetInt("bloomTex", 1);


        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, GetWindow().GetGBuffer()->GetDepth());
        m_combineShader->SetInt("depthTex", 2);

        GetWindow().SetFullViewport();

        // Write GBuffer depth into GAME_OUT (shader sets gl_FragDepth from depthTex).
        // Gizmos and particles both depend on this for correct depth testing.
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_ALWAYS);

        // Render fullscreen quad
        glBindVertexArray(GetRenderer().quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);

        Framebuffer::Unbind();
    }

}