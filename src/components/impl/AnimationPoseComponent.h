//
// Created by gabe on 6/30/25.
//

#ifndef CPP_ENGINE_ANIMATIONPOSECOMPONENT_H
#define CPP_ENGINE_ANIMATIONPOSECOMPONENT_H

#include "components/Components.h"

namespace Engine::Components {
	class AnimationPoseComponent : public Component {
	  public:
		std::vector<ozz::math::SoaTransform>* local_pose = nullptr;
		std::vector<ozz::math::Float4x4>*     model_pose = nullptr;

		AnimationPoseComponent() = default;

		void OnAdded(Entity& entity) override;
		void RenderInspector(Entity& entity) override;
	};
} // namespace Engine::Components

#endif // CPP_ENGINE_ANIMATIONPOSECOMPONENT_H
