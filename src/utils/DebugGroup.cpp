//
// Created by Gabe on 1/26/2026.
//

#include "DebugGroup.h"


namespace Engine {

    DebugGroup::DebugGroup(const char* name){
#ifndef GAME_BUILD
        glPushDebugGroup(
                GL_DEBUG_SOURCE_APPLICATION,
                0,
                -1,
                name
        );
#endif
    }
    DebugGroup::~DebugGroup()
        {
#ifndef GAME_BUILD
            glPopDebugGroup();
#endif
        }
}