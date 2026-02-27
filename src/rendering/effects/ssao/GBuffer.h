//
// Created by Gabe on 1/25/2026.
//

#pragma once

typedef unsigned int GLuint;
typedef int          GLint;

namespace Engine {
    class GBuffer {
    public:
        void Init(int width, int height);

        void Resize(int width, int height);

        void Bind() const;

        static void Unbind();

        void Delete();

        [[nodiscard]] GLuint GetAlbedo() const { return gAlbedo; }

        [[nodiscard]] GLuint GetNormal() const { return gNormal; }

        [[nodiscard]] GLuint GetMaterial() const { return gMaterial; }

        [[nodiscard]] GLuint GetEmissive() const { return gEmissive; }

        [[nodiscard]] GLuint GetDepth() const { return gDepth; }

        [[nodiscard]] GLuint GetFBO() const { return FBO; }

    private:
        GLuint FBO = 0;

        GLuint gAlbedo = 0;
        GLuint gNormal = 0;
        GLuint gMaterial = 0;
        GLuint gEmissive = 0;
        GLuint gDepth = 0;

        int width = 0;
        int height = 0;
    };
}
