//
// Created by gabe on 8/21/25.
//

#include "PlayerController.h"
#include "PlayerSettings.h"
#include "PhysicsInterfaces.h"
#include "imgui.h"

#include "core/EngineData.h"
#include "Camera.h"

namespace Engine {

	std::shared_ptr<CharacterVirtual> PlayerController::InitPlayer(std::shared_ptr<PhysicsSystem> physics, std::shared_ptr<TempAllocatorImpl> allocater)
	{
		RefConst<Shape>               mStandingShape = new CapsuleShape(cCharacterHalfHeight, cCharacterRadius);
		Ref<CharacterVirtualSettings> settings       = new CharacterVirtualSettings();
		settings->mMaxSlopeAngle                     = cMaxSlopeAngle;
		settings->mMaxStrength                       = cCharacterStrength;
		settings->mShape                             = mStandingShape;
		settings->mBackFaceMode                      = EBackFaceMode::CollideWithBackFaces;
		settings->mCharacterPadding                  = 0.02f;
		settings->mPenetrationRecoverySpeed          = 1.0f;
		settings->mPredictiveContactDistance         = 0.1f;
		settings->mSupportingVolume                  = Plane(Vec3::sAxisY(), -0.3f); // Accept contacts that touch the lower sphere of the capsule


		mCharacter = std::make_shared<CharacterVirtual>(settings, RVec3(0, 0, 0), Quat::sIdentity(), 0, physics.get());
		// TODO implement contact listener

		// CharacterContactListener listener = PlayerContactListener();
		// mCharacter->SetListener(&listener);
		return mCharacter;
	}


	void PlayerController::Update(std::shared_ptr<CharacterVirtual> mCharacter, std::shared_ptr<PhysicsSystem> physics, std::shared_ptr<TempAllocatorImpl> allocater, float dt)
	{
		Vec3 mDesiredVelocity = Vec3::sZero();

		// Update the character rotation and its up vector to match the up vector set by the user settings
		Quat character_up_rotation = Quat::sEulerAngles(Vec3(sUpRotationX, 0, sUpRotationZ));
		mCharacter->SetUp(character_up_rotation.RotateAxisY());
		mCharacter->SetRotation(character_up_rotation);

		// A cheaper way to update the character's ground velocity,
		// the platforms that the character is standing on may have changed velocity
		mCharacter->UpdateGroundVelocity();


		// Settings for our update function
		CharacterVirtual::ExtendedUpdateSettings update_settings;
		update_settings.mStickToFloorStepDown = -mCharacter->GetUp() * update_settings.mStickToFloorStepDown.Length();
		update_settings.mWalkStairsStepUp     = mCharacter->GetUp() * update_settings.mWalkStairsStepUp.Length();
		// Update the character position
		mCharacter->ExtendedUpdate(dt, Vec3(0, -9.8, 0), update_settings, physics->GetDefaultBroadPhaseLayerFilter(Layers::MOVING), physics->GetDefaultLayerFilter(Layers::MOVING), {}, {}, *allocater);
	}


	glm::vec3 PlayerController::GetPlayerPosition()
	{
		Vec3 p = mCharacter->GetPosition();
		return glm::vec3(p.GetX(), p.GetY(), p.GetZ());
	}


} // namespace Engine