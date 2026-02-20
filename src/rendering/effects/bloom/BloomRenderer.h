//
// Created by Gabe on 2/19/2026.
//
#pragma once

#include "BloomMip.h"
#include "rendering/Shader.h"

namespace Engine {
    class BloomRenderer {
    public:
        void Initialize();
        void ReloadShaders();
        void RenderBloom();
        void ResizeFramebuffers(int render_width, int render_height);

        std::vector<BloomMip> GetBloomMips() const { return m_bloomMips; }
    private:
        void DownsampleChain();
        void UpsampleChain();
        void CombineScene();

        std::shared_ptr<Shader> m_downSampleShader;
        std::shared_ptr<Shader> m_upSampleShader;
        std::shared_ptr<Shader> m_combineShader;
        std::vector<BloomMip>   m_bloomMips;
    };
}