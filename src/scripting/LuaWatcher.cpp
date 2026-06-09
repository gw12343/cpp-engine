//
// Created by Gabe on 6/8/2026.
//

#include "LuaWatcher.h"

#include "ScriptManager.h"

namespace Engine
{
    void LuaWatcher::handleFileAction(efsw::WatchID watchid, const std::string& dir, const std::string& filename, efsw::Action action, const std::string& oldFilename)
    {
        if (filename.size() > 4 && filename.substr(filename.size() - 4) == ".lua") {
            // TODO editor script folder? not hard code random file ... tsk tsk
            if (filename != "init.lua") return;
            GetScriptManager().log->info("Reloading editor script");
            GetScriptManager().ReloadEditorScript();
        }
    }
}