//
// Created by gabe on 6/24/25.
//

#include "Components.h"
#include "impl/AnimationComponent.h"
#include "impl/TransformComponent.h"
#include "impl/LuaScriptComponent.h"
#include "impl/EntityMetadataComponent.h"
#include "impl/ShadowCasterComponent.h"
#include "impl/ModelRendererComponent.h"
#include "impl/RigidBodyComponent.h"
#include "impl/AnimationPoseComponent.h"
#include "impl/AnimationWorkerComponent.h"
#include "impl/AudioSourceComponent.h"
#include "impl/ParticleSystemComponent.h"
#include "impl/SkeletonComponent.h"
#include "impl/SkinnedMeshComponent.h"


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