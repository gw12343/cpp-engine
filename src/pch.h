//
// Created by Gabe on 2/20/2026.
//

#pragma once

// Stable third-party only. Project headers in the PCH force a full rebuild
// whenever those headers (or anything they include) change.

// Std
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

// GL / ECS / profiler
#include <glad/glad.h>
#include <entt/entt.hpp>
#include <tracy/Tracy.hpp>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

// ImGui
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"

// Logging / serialization (used by most TUs)
#include <spdlog/spdlog.h>
#include <cereal/cereal.hpp>
