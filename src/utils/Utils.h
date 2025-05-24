#pragma once
#include "Effekseer/Effekseer.Matrix44.h"

#include <glm/glm.hpp>
#include <ozz/base/containers/vector.h>
#include <ozz/base/maths/simd_math.h>
#include <ozz/base/maths/soa_transform.h>

namespace Engine {
	ozz::math::Float4x4   FromMatrix(const glm::mat4& glmMatrix);
	::Effekseer::Matrix44 ConvertGLMToEffekseerMatrix(const glm::mat4& glmMatrix);
} // namespace Engine