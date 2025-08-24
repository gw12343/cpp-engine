//
// Created by gabe on 8/24/25.
//

#ifndef CPP_ENGINE_CONSOLEWINDOW_H
#define CPP_ENGINE_CONSOLEWINDOW_H

#include "utils/ImGuiLogSink.h"

namespace Engine {
	void DrawConsoleWindow(std::shared_ptr<ImGuiLogSink> sink, bool* pOpen);
}
#endif // CPP_ENGINE_CONSOLEWINDOW_H
