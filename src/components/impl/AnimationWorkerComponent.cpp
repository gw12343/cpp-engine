#include "components/Components.h"

#include "core/Engine.h"
#include "core/Entity.h"
#include "utils/Utils.h"

#include "imgui.h"
#include "ozz/animation/runtime/sampling_job.h"
#include "ozz/animation/runtime/track.h"
#include "ozz/base/containers/vector.h"
#include "rendering/particles/ParticleManager.h"
#include "animation/AnimationManager.h"
#include "scripting/ScriptManager.h"

namespace Engine::Components {


	std::unordered_set<ozz::animation::SamplingJob::Context*> AnimationWorkerComponent::s_contexts;

	void AnimationWorkerComponent::CleanAnimationContexts()
	{
		for (ozz::animation::SamplingJob::Context* ctx : s_contexts) {
			delete ctx;
		}
	}

	void AnimationWorkerComponent::OnAdded(Entity& entity)
	{
		context = new ozz::animation::SamplingJob::Context();
		s_contexts.insert(context);
		// Get the animation component, then resize context for correct number of tracks
		ENGINE_ASSERT(context, "AnimationWorkerComponent::OnAdded: Failed to allocate SamplingJob::Context");

		ENGINE_VERIFY(entity.HasComponent<AnimationComponent>(), "AnimationWorkerComponent::OnAdded: Missing AnimationComponent");
		auto& animationComponent = entity.GetComponent<AnimationComponent>();
		ENGINE_VERIFY(animationComponent.animation, "AnimationWorkerComponent::OnAdded: AnimationComponent has null animation");

		context->Resize(animationComponent.animation->num_tracks());
		SPDLOG_INFO("Resized context for {} tracks", animationComponent.animation->num_tracks());
	}

	void AnimationWorkerComponent::RenderInspector(Entity& entity)
	{
		ImGui::Text("Sampling Job Context: %s", context ? "Initialized" : "Null");
	}

	
} // namespace Engine::Components
