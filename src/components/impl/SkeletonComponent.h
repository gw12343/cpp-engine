//
// Created by gabe on 6/30/25.
//

#ifndef CPP_ENGINE_SKELETONCOMPONENT_H
#define CPP_ENGINE_SKELETONCOMPONENT_H

#include "components/Components.h"

namespace Engine::Components {
	class SkeletonComponent : public Component {
	  public:
		ozz::animation::Skeleton* skeleton = nullptr;
		std::string               skeletonPath;

		SkeletonComponent() = default;
		explicit SkeletonComponent(ozz::animation::Skeleton* skeleton) : skeleton(skeleton) {}
		explicit SkeletonComponent(std::string skeletonPath) : skeletonPath(std::move(skeletonPath)) {}

		void OnAdded(Entity& entity) override;
		void RenderInspector(Entity& entity) override;
	};
} // namespace Engine::Components

#endif // CPP_ENGINE_SKELETONCOMPONENT_H
