//
// Created by Gabe on 1/25/2026.
//

#include "GBuffer.h"
#include "core/EngineData.h"
#include "Renderer.h"
#include "glad/glad.h"

namespace Engine {

        static GLuint CreateColorTexture(GLenum internalFormat, GLenum format, GLenum type, int w, int h)
        {
            GLuint tex;
            glGenTextures(1, &tex);
            glBindTexture(GL_TEXTURE_2D, tex);
            glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(internalFormat), w, h, 0, format, type, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            return tex;
        }

        void GBuffer::Init(int w, int h)
        {
            width = w;
            height = h;

            glGenFramebuffers(1, &FBO);
            glBindFramebuffer(GL_FRAMEBUFFER, FBO);

            // Albedo + alpha
            gAlbedo = CreateColorTexture(GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, w, h);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gAlbedo, 0);

            // Normal (world space)
            gNormal = CreateColorTexture(GL_RGB16F, GL_RGB, GL_FLOAT, w, h);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);

            // Material (specular strength in R)
            gMaterial = CreateColorTexture(GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, w, h);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gMaterial, 0);

            // Emissive
            gEmissive = CreateColorTexture(GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE, w, h);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, gEmissive, 0);

            // Depth (texture, not renderbuffer!)
            glGenTextures(1, &gDepth);
            glBindTexture(GL_TEXTURE_2D, gDepth);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, w, h, 0,
                         GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, gDepth, 0);

            GLenum attachments[] = {
                    GL_COLOR_ATTACHMENT0,
                    GL_COLOR_ATTACHMENT1,
                    GL_COLOR_ATTACHMENT2,
                    GL_COLOR_ATTACHMENT3
            };
            glDrawBuffers(4, attachments);

            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                GetRenderer().log->error("GBuffer framebuffer incomplete!");
            }

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        void GBuffer::Resize(int w, int h)
        {
            Delete();
            Init(w, h);
        }

        void GBuffer::Bind() const
        {
            glBindFramebuffer(GL_FRAMEBUFFER, FBO);
        }

        void GBuffer::Unbind()
        {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        void GBuffer::Delete()
        {
            glDeleteFramebuffers(1, &FBO);
            glDeleteTextures(1, &gAlbedo);
            glDeleteTextures(1, &gNormal);
            glDeleteTextures(1, &gMaterial);
            glDeleteTextures(1, &gEmissive);
            glDeleteTextures(1, &gDepth);

            FBO = gAlbedo = gNormal = gMaterial = gEmissive = gDepth = 0;
        }

}
