//
// Created by Gabe on 6/8/2026.
//

#pragma once

#include "efsw/efsw.hpp"

namespace Engine
{
    class LuaWatcher : public efsw::FileWatchListener {
    public:
        void handleFileAction(efsw::WatchID watchid, const std::string& dir, const std::string& filename, efsw::Action action, const std::string& oldFilename) override;
    };
}