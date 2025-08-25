#pragma once

#include "components/Components.h"


using namespace JPH;
using namespace JPH::literals;

namespace Layers {
	static constexpr ObjectLayer NON_MOVING = 0;
	static constexpr ObjectLayer MOVING     = 1;
	static constexpr ObjectLayer NUM_LAYERS = 2;
}; // namespace Layers

namespace BroadPhaseLayers {
	static constexpr BroadPhaseLayer NON_MOVING(0);
	static constexpr BroadPhaseLayer MOVING(1);
	static constexpr uint            NUM_LAYERS(2);
}; // namespace BroadPhaseLayers

namespace Engine {

	class ObjectLayerPairFilterImpl : public ObjectLayerPairFilter {
	  public:
		[[nodiscard]] bool ShouldCollide(ObjectLayer inObject1, ObjectLayer inObject2) const override;
	};

	class BPLayerInterfaceImpl final : public BroadPhaseLayerInterface {
	  public:
		BPLayerInterfaceImpl();

		[[nodiscard]] uint            GetNumBroadPhaseLayers() const override;
		[[nodiscard]] BroadPhaseLayer GetBroadPhaseLayer(ObjectLayer inLayer) const override;
		[[nodiscard]] const char*     GetBroadPhaseLayerName(BroadPhaseLayer inLayer) const override;


	  private:
		BroadPhaseLayer mObjectToBroadPhase[Layers::NUM_LAYERS];
	};

	class ObjectVsBroadPhaseLayerFilterImpl : public ObjectVsBroadPhaseLayerFilter {
	  public:
		[[nodiscard]] bool ShouldCollide(ObjectLayer inLayer1, BroadPhaseLayer inLayer2) const override;
	};

	class ContactListenerImpl : public ContactListener {
	  public:
		ValidateResult OnContactValidate(const Body& inBody1, const Body& inBody2, RVec3Arg inBaseOffset, const CollideShapeResult& inCollisionResult) override;
		void           OnContactAdded(const Body& inBody1, const Body& inBody2, const ContactManifold& inManifold, ContactSettings& ioSettings) override;
		void           OnContactPersisted(const Body& inBody1, const Body& inBody2, const ContactManifold& inManifold, ContactSettings& ioSettings) override;
		void           OnContactRemoved(const SubShapeIDPair& inSubShapePair) override;
	};

	class BodyActivationListenerImpl : public BodyActivationListener {
	  public:
		void OnBodyActivated(const BodyID& inBodyID, uint64 inBodyUserData) override;

		void OnBodyDeactivated(const BodyID& inBodyID, uint64 inBodyUserData) override;
	};

} // namespace Engine