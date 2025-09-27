//
// Created by gabe on 6/24/25.
//

#include "components/Components.h"
#include "components/impl/AnimationComponent.h"

#include "core/Engine.h"
#include "core/Entity.h"
#include "utils/Utils.h"
#include "imgui.h"
#include "ozz/animation/runtime/track.h"
#include "rendering/particles/ParticleManager.h"
#include "animation/AnimationManager.h"
#include "scripting/ScriptManager.h"

#include "assets/AssetManager.h"

#include "rendering/ui/InspectorUI.h"
#include "animation/Animation.h"
#include "AnimationPoseComponent.h"
#include "SkeletonComponent.h"

namespace Engine::Components {


	std::unordered_set<ozz::animation::SamplingJob::Context*> AnimationComponent::s_contexts;

	void AnimationComponent::CleanAnimationContexts()
	{
		for (ozz::animation::SamplingJob::Context* ctx : s_contexts) {
			delete ctx;
		}
	}


	void AnimationComponent::OnAdded(Entity& entity)
	{
		context = new ozz::animation::SamplingJob::Context();

		s_contexts.insert(context);
		SetAnimation(animation);
	}


	void AnimationComponent::SetAnimation(AssetHandle<Animation> animation)
	{
		this->animation = animation;

		if (animation.IsValid()) {
			Animation* anim = GetAssetManager().Get(animation);

			if (anim && anim->source) {
				context->Resize(anim->source->num_tracks());
				GetDefaultLogger()->info("Resized context for {} tracks", anim->source->num_tracks());
			}
		}
	}

	void AnimationComponent::OnRemoved(Entity& entity)
	{
		s_contexts.erase(context);
		delete context;
		context = nullptr;
	}


	void AnimationComponent::RenderInspector(Entity& entity)
	{
		{
			Animation* anim = GetAssetManager().Get(animation);

			ImGui::Text("Animation: %s", anim->source ? "Loaded" : "Null");
			ImGui::Text("Tracks: %d", anim->source ? anim->source->num_tracks() : 0);
			ImGui::Text("Duration: %.2f", anim->source ? anim->source->duration() : 0.0f);
			ImGui::Separator();
		}


		if (LeftLabelSliderFloat("Time scale", &timescale, 0.0, 1.0)) {
			if (entity.HasComponent<Components::AnimationPoseComponent>() && entity.HasComponent<Components::SkeletonComponent>()) {
				auto& animationPoseComponent = entity.GetComponent<Components::AnimationPoseComponent>();
				auto& skeletonComponent      = entity.GetComponent<Components::SkeletonComponent>();

				// Samples optimized animation at t = animation_time_
				ozz::animation::SamplingJob sampling_job;
				sampling_job.animation = GetAssetManager().Get(animation)->source;

				sampling_job.context = context;
				sampling_job.ratio   = timescale;
				sampling_job.output  = ozz::make_span(*animationPoseComponent.local_pose);
				if (!sampling_job.Run()) {
					GetAnimationManager().log->error("Failed to sample animation");
					return;
				}

				ozz::animation::LocalToModelJob ltm_job;
				ltm_job.skeleton = skeletonComponent.skeleton;
				ltm_job.input    = ozz::make_span(*animationPoseComponent.local_pose);
				ltm_job.output   = ozz::make_span(*animationPoseComponent.model_pose);
				if (!ltm_job.Run()) {
					GetAnimationManager().log->error("Failed to convert to model space");
					return;
				}
			}
		}


		AssetHandle<Animation> anim = animation;
		if (LeftLabelAssetAnimation("Animation", &anim)) {
			SetAnimation(anim);
			//			if (!animation.IsValid() || !GetAssetManager().Get(animation)) {
			//				spdlog::error("Failed to load animation: {}", animation.GetID());
			//			}
			//			else {
			//				GetDefaultLogger()->info("Loaded animation: {}", animation.GetID());
			//			}
		}
	}


} // namespace Engine::Components