//
// Created by gabe on 8/21/25.
//

#include "PlayerController.h"
#include "PlayerSettings.h"
#include "PhysicsInterfaces.h"


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


		mCharacter = std::make_shared<CharacterVirtual>(settings, RVec3(-40, 10, 4), Quat::sIdentity(), 0, physics.get());
		// TODO implement contact listener

		// CharacterContactListener listener = PlayerContactListener();
		// mCharacter->SetListener(&listener);
		return mCharacter;
	}


	void PlayerController::Update(std::shared_ptr<CharacterVirtual> mCharacter, std::shared_ptr<PhysicsSystem> physics, std::shared_ptr<TempAllocatorImpl> allocater, float dt)
	{
		Vec3 mDesiredVelocity = Vec3::sZero();
		bool mAllowSliding    = false;
		{
			bool player_controls_horizontal_velocity = true; // mCharacter->IsSupported();
			if (player_controls_horizontal_velocity) {
				float sCharacterSpeed = 25.0f;

				Vec3 inMovementDirection(0, 0, 0);


				// TODO input handling, maybe poll script?

				//				{
				//					glm::vec3 Orientation = Camera::GetOrientation();
				//
				//
				//					float xAxis = 0;
				//					float yAxis = 0;
				//					yAxis += ImGui::IsKeyDown(ImGuiKey_W) ? 1 : 0;
				//					yAxis -= ImGui::IsKeyDown(ImGuiKey_S) ? 1 : 0;
				//					xAxis += ImGui::IsKeyDown(ImGuiKey_A) ? 1 : 0;
				//					xAxis -= ImGui::IsKeyDown(ImGuiKey_D) ? 1 : 0;
				//
				//
				//					glm::vec3 mov(0, 0, 0);
				//					mov -= glm::vec3(glm::sin(Orientation.y), 0, glm::cos(Orientation.y)) * yAxis;
				//					mov -= glm::vec3(glm::sin(Orientation.y + glm::pi<float>() / 2.0), 0, glm::cos(Orientation.y + glm::pi<float>() / 2.0)) * xAxis;
				//					// mov = glm::normalize(mov);
				//					inMovementDirection = {mov.x, 0, mov.z};
				//				}


				// Smooth the player input
				mDesiredVelocity = 0.25f * inMovementDirection * sCharacterSpeed + 0.75f * mDesiredVelocity;

				// True if the player intended to move
				mAllowSliding = !inMovementDirection.IsNearZero();
			}
			else {
				// While in air we allow sliding
				mAllowSliding = true;
			}

			// Update the character rotation and its up vector to match the up vector set by the user settings
			Quat character_up_rotation = Quat::sEulerAngles(Vec3(sUpRotationX, 0, sUpRotationZ));
			mCharacter->SetUp(character_up_rotation.RotateAxisY());
			mCharacter->SetRotation(character_up_rotation);

			// A cheaper way to update the character's ground velocity,
			// the platforms that the character is standing on may have changed velocity
			mCharacter->UpdateGroundVelocity();

			// Determine new basic velocity
			Vec3 current_vertical_velocity = mCharacter->GetLinearVelocity().Dot(mCharacter->GetUp()) * mCharacter->GetUp();
			Vec3 ground_velocity           = mCharacter->GetGroundVelocity();
			Vec3 new_velocity;
			bool moving_towards_ground = (current_vertical_velocity.GetY() - ground_velocity.GetY()) < 0.1f;

			bool inJump = false; // Camera::cameraType == CameraType::FPS && ImGui::IsKeyPressed(ImGuiKey_Space);

			if (mCharacter->GetGroundState() == CharacterVirtual::EGroundState::OnGround // If on ground
			    && (true ? moving_towards_ground                                         // Inertia enabled: And not moving away from ground
			             : !mCharacter->IsSlopeTooSteep(mCharacter->GetGroundNormal()))) // Inertia disabled: And not on a slope that is too steep
			{
				// Assume velocity of ground when on ground
				new_velocity = ground_velocity;

				// Jump
				if (inJump && moving_towards_ground) new_velocity += cCharacterJumpPower * mCharacter->GetUp();
			}
			else
				new_velocity = current_vertical_velocity;

			// Gravity
			// new_velocity += (character_up_rotation * physics->GetGravity()) * dt;

			if (player_controls_horizontal_velocity) {
				// Player input
				new_velocity += character_up_rotation * mDesiredVelocity;
			}
			else {
				// Preserve horizontal velocity
				Vec3 current_horizontal_velocity = mCharacter->GetLinearVelocity() - current_vertical_velocity;
				new_velocity += current_horizontal_velocity;
			}

			// Update character velocity
			mCharacter->SetLinearVelocity(new_velocity);


			// Settings for our update function
			CharacterVirtual::ExtendedUpdateSettings update_settings;
			if (!true)
				update_settings.mStickToFloorStepDown = Vec3::sZero();
			else
				update_settings.mStickToFloorStepDown = -mCharacter->GetUp() * update_settings.mStickToFloorStepDown.Length();
			if (!true)
				update_settings.mWalkStairsStepUp = Vec3::sZero();
			else
				update_settings.mWalkStairsStepUp = mCharacter->GetUp() * update_settings.mWalkStairsStepUp.Length();

			// Update the character position
			mCharacter->ExtendedUpdate(dt, Vec3(0, -9.8, 0), update_settings, physics->GetDefaultBroadPhaseLayerFilter(Layers::MOVING), physics->GetDefaultLayerFilter(Layers::MOVING), {}, {}, *allocater);


			Vec3 pos = mCharacter->GetPosition();
			//			if (Camera::cameraType == FPS) {
			//				Camera::SetPosition({ pos.GetX(), pos.GetY() + 1.45, pos.GetZ() });
			//			}
		}
	}


	glm::vec3 PlayerController::getPlayerPosition()
	{
		Vec3 p = mCharacter->GetPosition();
		return glm::vec3(p.GetX(), p.GetY(), p.GetZ());
	}
} // namespace Engine