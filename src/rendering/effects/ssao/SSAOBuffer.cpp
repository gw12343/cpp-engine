//
// Created by Gabe on 1/29/2026.
//

#include "core/EngineData.h"
#include "rendering/Renderer.h"

#include "SSAOBuffer.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>


namespace Engine {
    void SSAOBuffer::Resize(int w, int h, bool halfRes)
    {
        Delete();

        width  = halfRes ? w / 2 : w;
        height = halfRes ? h / 2 : h;

        /* ---------------- SSAO Noise Texture ---------------------*/

        std::vector<glm::vec3> noise;
        for (int i = 0; i < 16; i++)
        {
            glm::vec3 n = glm::normalize(glm::vec3(
                    glm::linearRand(-1.0f, 1.0f),
                    glm::linearRand(-1.0f, 1.0f),
                    0.0f
            ));
            noise.push_back(n);
        }


        glGenTextures(1, &noiseTex);
        glBindTexture(GL_TEXTURE_2D, noiseTex);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, 4, 4, 0, GL_RGB, GL_FLOAT, noise.data());

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);



        /* ---------------- SSAO FBO ---------------- */

        glGenFramebuffers(1, &ssaoFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);

        glGenTextures(1, &ssaoTex);
        glBindTexture(GL_TEXTURE_2D, ssaoTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, width, height, 0, GL_RED, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoTex, 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            GetRenderer().log->error("SSAO FBO not complete!");

        /* ---------------- Blur FBO ---------------- */

        glGenFramebuffers(1, &blurFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, blurFBO);

        glGenTextures(1, &blurTex);
        glBindTexture(GL_TEXTURE_2D, blurTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, width, height, 0, GL_RED, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, blurTex, 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            GetRenderer().log->error("SSAO Blur FBO not complete!");

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void SSAOBuffer::BindSSAO() const
    {
        glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
        glViewport(0, 0, width, height);
    }

    void SSAOBuffer::BindBlur() const
    {
        glBindFramebuffer(GL_FRAMEBUFFER, blurFBO);
        glViewport(0, 0, width, height);
    }

    void SSAOBuffer::Unbind() const
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void SSAOBuffer::Delete()
    {
        if (glfwGetCurrentContext() != nullptr) {
            if (ssaoFBO) glDeleteFramebuffers(1, &ssaoFBO);
            if (blurFBO) glDeleteFramebuffers(1, &blurFBO);
            if (ssaoTex) glDeleteTextures(1, &ssaoTex);
            if (blurTex) glDeleteTextures(1, &blurTex);
            if (noiseTex) glDeleteTextures(1, &noiseTex);
        }

        ssaoFBO = blurFBO = 0;
        ssaoTex = blurTex = noiseTex = 0;
    }
}