//
// Created by Gabe on 6/8/2026.
//

#pragma once


namespace sol
{
    class state;
}

namespace Engine
{
    class ComponentMethodBinder {
    public:
        void BindMethodsLua(sol::state* state);
    };
}


