//
// Created by Gabe on 1/21/2026.
//

#pragma once

#include "core/module/Module.h"
#include "core/Entity.h"

#ifdef VR



namespace Engine {
    class VRModule : public Module {
    public:
        void                      onInit() override;
        void                      onUpdate(float dt) override;
        void                      onGameStart() override;
        void                      onShutdown() override;

        [[nodiscard]] std::string name() const override { return "VRModule"; }



        void waitForXRFrame();
        void endXRFrame();
    };
}


#endif