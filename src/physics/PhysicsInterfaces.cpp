#include "physics/PhysicsInterfaces.h"
#include "core/EngineData.h"
#include "physics/PhysicsManager.h"
#include "components/impl/LuaScriptComponent.h"

using namespace JPH;
using namespace JPH::literals;

namespace Engine {
	bool ObjectLayerPairFilterImpl::ShouldCollide(ObjectLayer inObject1, ObjectLayer inObject2) const
	{
		switch (inObject1) {
			case Layers::NON_MOVING:
				// Non moving only collides with moving
				return inObject2 == Layers::MOVING;
			case Layers::MOVING:
				// Moving collides with everything
				return true;
			default:
				JPH_ASSERT(false);
				return false;
		}
	}


	BPLayerInterfaceImpl::BPLayerInterfaceImpl()
	{
		// Create a mapping table from object to broad phase layer
		mObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
		mObjectToBroadPhase[Layers::MOVING]     = BroadPhaseLayers::MOVING;
	}

	uint BPLayerInterfaceImpl::GetNumBroadPhaseLayers() const
	{
		return BroadPhaseLayers::NUM_LAYERS;
	}

	BroadPhaseLayer BPLayerInterfaceImpl::GetBroadPhaseLayer(ObjectLayer inLayer) const
	{
		JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
		return mObjectToBroadPhase[inLayer];
	}


	const char* BPLayerInterfaceImpl::GetBroadPhaseLayerName(BroadPhaseLayer inLayer) const
	{
		switch ((BroadPhaseLayer::Type) inLayer) {
			case (BroadPhaseLayer::Type) BroadPhaseLayers::NON_MOVING:
				return "NON_MOVING";
			case (BroadPhaseLayer::Type) BroadPhaseLayers::MOVING:
				return "MOVING";
			default:
				JPH_ASSERT(false);
				return "INVALID";
		}
	}


	bool ObjectVsBroadPhaseLayerFilterImpl::ShouldCollide(ObjectLayer inLayer1, BroadPhaseLayer inLayer2) const
	{
		switch (inLayer1) {
			case Layers::NON_MOVING:
				return inLayer2 == BroadPhaseLayers::MOVING;
			case Layers::MOVING:
				return true;
			default:
				JPH_ASSERT(false);
				return false;
		}
	}


	// See: ContactListener
	ValidateResult ContactListenerImpl::OnContactValidate(const Body& inBody1, const Body& inBody2, RVec3Arg inBaseOffset, const CollideShapeResult& inCollisionResult)
	{
		// SPDLOG_INFO("Contact validate callback");

		// Allows you to ignore a contact before it is created (using layers to not make objects collide is cheaper!)
		return ValidateResult::AcceptAllContactsForThisBodyPair;
	}

	void ContactListenerImpl::OnContactAdded(const Body& inBody1, const Body& inBody2, const ContactManifold& inManifold, ContactSettings& ioSettings)
	{
		Entity& entity1 = GetPhysics().bodyToEntityMap[inBody1.GetID()];
		Entity& entity2 = GetPhysics().bodyToEntityMap[inBody2.GetID()];

		if (entity1.HasComponent<Components::LuaScript>()) {
			auto& sc1 = entity1.GetComponent<Components::LuaScript>();
			sc1.OnCollisionEnter(entity2);
		}

		if (entity2.HasComponent<Components::LuaScript>()) {
			auto& sc2 = entity2.GetComponent<Components::LuaScript>();
			sc2.OnCollisionEnter(entity1);
		}
	}

	void ContactListenerImpl::OnContactPersisted(const Body& inBody1, const Body& inBody2, const ContactManifold& inManifold, ContactSettings& ioSettings)
	{
		// SPDLOG_INFO("A contact was persisted");
	}

	void ContactListenerImpl::OnContactRemoved(const SubShapeIDPair& inSubShapePair)
	{
		// SPDLOG_INFO("A contact was removed");
	}


	void BodyActivationListenerImpl::OnBodyActivated(const BodyID& inBodyID, uint64 inBodyUserData)
	{
		// SPDLOG_INFO("\033[1;34mA body got activated\033[0m");
	}

	void BodyActivationListenerImpl::OnBodyDeactivated(const BodyID& inBodyID, uint64 inBodyUserData)
	{
		// SPDLOG_INFO("\033[1;34mA body went to sleep\033[0m");
	}
} // namespace Engine