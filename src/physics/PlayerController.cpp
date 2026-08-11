//
// Created by gabe on 8/21/25.
//

#include "PlayerController.h"
#include "PhysicsManager.h"
#include "PlayerSettings.h"
#include "PhysicsInterfaces.h"


#include "core/EngineData.h"
#include "Camera.h"

#include "components/impl/LuaScriptComponent.h"
#include "scripting/ScriptManager.h"

namespace Engine {

	class PlayerContactListener : public CharacterContactListener {
	  public:
		// Called whenever the character touches another body
		bool OnContactValidate(const CharacterVirtual *inCharacter, const CharacterContact &inContact) override
		{

			// Decide whether this contact should be valid or ignored
			// e.g., ignore triggers, or prevent standing on certain objects
			// Return true to accept the contact, false to reject it.
			return true;
		}

		// Called whenever a contact is added
		void OnContactAdded(const CharacterVirtual *inCharacter, const CharacterContact &inContact, CharacterContactSettings &ioSettings) override
		{
			auto& physics       = GetPhysics();
			auto& scriptManager = GetScriptManager();



			Entity& entity1 = physics.bodyToEntityMap[inContact.mBodyB];

			if (!entity1) {
				ENGINE_WARN("SHOULD NOT BE NULL");
				return;
			}

			if (entity1.HasComponent<Components::LuaScript>()) {
				std::lock_guard<std::mutex> lock(scriptManager.collisionMutex);
				scriptManager.pendingCharacterCollisions.push_back(entity1);
			}
		}

		// Called whenever a contact is persisted
		void OnContactPersisted(const CharacterVirtual *inCharacter, const CharacterContact &inContact, CharacterContactSettings &ioSettings) override
		{
			// You can modify ongoing contact behavior here
		}

		// Called whenever a contact is removed
		void OnContactRemoved(const CharacterVirtual* inCharacter, const BodyID& inBodyID, const SubShapeID& inSubShapeID) override
		{
			// Cleanup or state updates when a contact ends
		}
	};

	std::shared_ptr<CharacterContactListener> contactListener;
	std::shared_ptr<CharacterVirtual>         PlayerController::InitPlayer(std::shared_ptr<PhysicsSystem> physics, std::shared_ptr<TempAllocatorImpl> allocater)
	{
		RefConst<Shape>               mStandingShape = new CapsuleShape(cCharacterHalfHeight, cCharacterRadius);
		Ref<CharacterVirtualSettings> settings       = new CharacterVirtualSettings();
		settings->mMaxSlopeAngle                     = cMaxSlopeAngle;
		settings->mMaxStrength                       = cCharacterStrength;
		settings->mShape                             = mStandingShape;
		settings->mBackFaceMode                      = EBackFaceMode::CollideWithBackFaces;
		settings->mCharacterPadding                  = 0.02f;
		settings->mPenetrationRecoverySpeed          = 0.5f;
		settings->mPredictiveContactDistance         = 0.1f;
		settings->mSupportingVolume                  = Plane(Vec3::sAxisY(), -0.3f); // Accept contacts that touch the lower sphere of the capsule


		mCharacter = std::make_shared<CharacterVirtual>(settings, RVec3(0, 0, 0), Quat::sIdentity(), 0, physics.get());
		// TODO implement contact listener

		contactListener = std::make_shared<PlayerContactListener>();
		mCharacter->SetListener(contactListener.get());
		return mCharacter;
	}


	void PlayerController::Update(std::shared_ptr<CharacterVirtual> mCharacter, std::shared_ptr<PhysicsSystem> physics, std::shared_ptr<TempAllocatorImpl> allocater, float dt)
	{
		// Keep character upright. Do not overwrite yaw — gameplay scripts set facing
		// (needed for third-person / animated characters). Climbing also stays upright
		// (BOTW-style: body up, movement projected onto the wall plane).
		Quat character_up_rotation = Quat::sEulerAngles(Vec3(sUpRotationX, 0, sUpRotationZ));
		mCharacter->SetUp(character_up_rotation.RotateAxisY());

		mCharacter->UpdateGroundVelocity();

		CharacterVirtual::ExtendedUpdateSettings update_settings;
		Vec3                                     gravity(0, -9.8f, 0);

		if (mClimbing) {
			// Attached to a wall: no gravity pull, no stick-to-floor / stair walk.
			// Script drives velocity along the wall; we only resolve collisions.
			gravity                                     = Vec3::sZero();
			update_settings.mStickToFloorStepDown       = Vec3::sZero();
			update_settings.mWalkStairsStepUp           = Vec3::sZero();
			update_settings.mWalkStairsStepForwardTest  = 0.0f;
			update_settings.mWalkStairsMinStepForward   = 0.0f;
		}
		else {
			update_settings.mStickToFloorStepDown = -mCharacter->GetUp() * update_settings.mStickToFloorStepDown.Length();
			update_settings.mWalkStairsStepUp     = mCharacter->GetUp() * update_settings.mWalkStairsStepUp.Length();
		}

		mCharacter->ExtendedUpdate(dt, gravity, update_settings, physics->GetDefaultBroadPhaseLayerFilter(Layers::MOVING), physics->GetDefaultLayerFilter(Layers::MOVING), {}, {}, *allocater);
	}


	glm::vec3 PlayerController::GetPlayerPosition()
	{
		Vec3 p = mCharacter->GetPosition();
		return glm::vec3(p.GetX(), p.GetY(), p.GetZ());
	}


} // namespace Engine