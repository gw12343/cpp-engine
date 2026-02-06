//
// Created by Gabe on 1/29/2026.
//

#pragma once

typedef unsigned int GLuint;
typedef int          GLint;

namespace Engine {

    class SSAOBuffer {
    public:
        void Resize(int width, int height, bool halfRes = true);
        void BindSSAO() const;
        void BindBlur() const;
        void Unbind() const;
        void Delete();

        GLuint ssaoFBO = 0;
        GLuint blurFBO = 0;

        GLuint ssaoTex = 0;
        GLuint blurTex = 0;


        GLuint noiseTex = 0;

        int width = 0;
        int height = 0;
    };

}