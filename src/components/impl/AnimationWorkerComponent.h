//
// Created by gabe on 6/30/25.
//

#ifndef CPP_ENGINE_ANIMATIONWORKERCOMPONENT_H
#define CPP_ENGINE_ANIMATIONWORKERCOMPONENT_H

#include "components/Components.h"

namespace Engine::Components {
	class AnimationWorkerComponent : public Component {
	  public:
		ozz::animation::SamplingJob::Context* context = nullptr;

		AnimationWorkerComponent() = default;

		void OnAdded(Entity& entity) override;
		void RenderInspector(Entity& entity) override;

		static void CleanAnimationContexts();

		static std::unordered_set<ozz::animation::SamplingJob::Context*> s_contexts;
	};
} // namespace Engine::Components

#endif // CPP_ENGINE_ANIMATIONWORKERCOMPONENT_H
