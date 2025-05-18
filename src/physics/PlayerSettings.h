#pragma once

#include "Jolt/Jolt.h"

using namespace JPH;

namespace Engine {

	static constexpr float	cCharacterStrength = 100.0f;
	static constexpr float	cMaxSlopeAngle = DegreesToRadians(45.0f);

	static constexpr float	cCharacterRadius = 0.5f;
	static constexpr float	cCharacterHalfHeight = 1.0f;
	static constexpr float	cCharacterJumpPower = 8.0f;

	static inline float		sUpRotationX = 0;
	static inline float		sUpRotationZ = 0;
}