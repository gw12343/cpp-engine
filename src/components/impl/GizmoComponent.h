//
// Created by gabe on 9/15/25.
//

#pragma once

#include "components/Components.h"
#include "cereal/cereal.hpp"
namespace Engine::Components {
	// An editor only visualization component
	class GizmoComponent : public Component {
	  public:
		float     radius = 0.1f;
		glm::vec3 color{1.0, 1.0, 1.0};

		GizmoComponent() = default;

		template <class Archive>
		void serialize(Archive& ar)
		{
			ar(CEREAL_NVP(radius), CEREAL_NVP(color));
		}


		void OnAdded(Entity& entity) override;
		void OnRemoved(Entity& entity) override;
		void RenderInspector(Entity& entity) override;

		static void AddBindings();
	};
} // namespace Engine::Components