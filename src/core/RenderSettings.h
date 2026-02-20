//
// Created by Gabe on 2/14/2026.
//

#pragma once

#include "glm/glm.hpp"
#include <vector>

namespace Engine {

    struct RenderSettings {
        unsigned int depthMapResolution = 4096;
        const glm::vec3           lightDir           = glm::normalize(glm::vec3(2.0f, 20, 2.0f));
        const float               CAMERA_NEAR_PLANE  = 0.1f;
        const float               CAMERA_FAR_PLANE   = 1500.0f;
        std::vector<float>        shadowCascadeLevels{CAMERA_FAR_PLANE / 100.0f, CAMERA_FAR_PLANE / 50.0f, CAMERA_FAR_PLANE / 25.0f, CAMERA_FAR_PLANE / 10.0f, CAMERA_FAR_PLANE / 2.0f};
        const unsigned int BLOOM_MIPS = 6;

        float bloom_threshold = 0.1f;
        float bloom_knee = 0.5f;
    };

}