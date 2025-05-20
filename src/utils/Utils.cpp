#include "utils/Utils.h"

namespace Engine {
	ozz::math::Float4x4 FromMatrix(const glm::mat4& glmMatrix)
	{
		return ozz::math::Float4x4{ozz::math::simd_float4::Load(glmMatrix[0][0], glmMatrix[0][1], glmMatrix[0][2], glmMatrix[0][3]),
		                           ozz::math::simd_float4::Load(glmMatrix[1][0], glmMatrix[1][1], glmMatrix[1][2], glmMatrix[1][3]),
		                           ozz::math::simd_float4::Load(glmMatrix[2][0], glmMatrix[2][1], glmMatrix[2][2], glmMatrix[2][3]),
		                           ozz::math::simd_float4::Load(glmMatrix[3][0], glmMatrix[3][1], glmMatrix[3][2], glmMatrix[3][3])};
	}
} // namespace Engine