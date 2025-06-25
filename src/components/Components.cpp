//
// Created by gabe on 6/24/25.
//

#include "Components.h"

namespace Engine::Components {

	void RegisterAllComponentBindings()
	{
		AnimationComponent::AddBindings();
		AnimationPoseComponent::AddBindings();
		AnimationWorkerComponent::AddBindings();
		AudioSource::AddBindings();
		EntityMetadata::AddBindings();
		LuaScript::AddBindings();
		ModelRenderer::AddBindings();
		ParticleSystem::AddBindings();
		RigidBodyComponent::AddBindings();
		ShadowCaster::AddBindings();
		SkeletonComponent::AddBindings();
		SkinnedMeshComponent::AddBindings();
		Transform::AddBindings();

		// TODO make this automatic?
	}
} // namespace Engine::Components