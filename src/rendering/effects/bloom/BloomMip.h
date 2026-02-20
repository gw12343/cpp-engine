//
// Created by Gabe on 2/19/2026.
//

#pragma once

#include <vector>
#include "glm/glm.hpp"
#include "rendering/Framebuffer.h"

namespace Engine {
    struct BloomMip {
        Framebuffer fb;
        glm::vec2 size;
    };
}

