//
// Created by Gabe on 2/20/2026.
//

#pragma once

// Std
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <filesystem>

// Engine
#include "core/EngineData.h"
#include "assets/AssetHandle.h"
#include "assets/AssetManager.h"


// Entt
#include <entt/entt.hpp>

// Utils
#include "utils/Utils.h"
#include "utils/DebugGroup.h"

// Tracy
#include <tracy/Tracy.hpp>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "glm/gtc/random.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/detail/type_quat.hpp"
#include "glm/gtc/quaternion.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/quaternion.hpp"

// ImGui
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "rendering/ui/InspectorUI.h"