//
// Created by gabe on 8/21/25.
//

#ifndef CPP_ENGINE_PLAYERCONTROLLER_H
#define CPP_ENGINE_PLAYERCONTROLLER_H

#include "glm/glm.hpp"

// Jolt includes
#include "Jolt/Jolt.h"
#include "Jolt/Physics/PhysicsScene.h"
#include "Jolt/Physics/Collision/Shape/CapsuleShape.h"
#include "Jolt/Physics/Collision/Shape/CylinderShape.h"
#include "Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h"
#include "Jolt/Physics/Collision/Shape/BoxShape.h"
#include "Jolt/Physics/Collision/Shape/SphereShape.h"
#include "Jolt/Physics/Collision/Shape/MeshShape.h"
#include "Jolt/Physics/Constraints/HingeConstraint.h"
#include "Jolt/Core/StringTools.h"
#include "Jolt/ObjectStream/ObjectStreamIn.h"
#include "Jolt/Physics/Character/CharacterVirtual.h"
#include "Jolt/Physics/Character/Character.h"
#include "Jolt/Physics/Character/CharacterBase.h"
#include "Jolt/Physics/Collision/Shape/ScaledShape.h"

#include <memory>

using namespace JPH;

namespace Engine {
	class PlayerController {
	  public:
		std::shared_ptr<CharacterVirtual> InitPlayer(std::shared_ptr<PhysicsSystem> physics, std::shared_ptr<TempAllocatorImpl> allocater);
		void                              Update(std::shared_ptr<CharacterVirtual> mCharacter, std::shared_ptr<PhysicsSystem> physics, std::shared_ptr<TempAllocatorImpl> allocater, float dt);
		glm::vec3                         getPlayerPosition();

	  private:
		std::shared_ptr<CharacterVirtual> mCharacter;
	};
} // namespace Engine


#endif // CPP_ENGINE_PLAYERCONTROLLER_H
