//
// Created by gabe on 9/15/25.
//

#include "GizmoComponent.h"
#include "rendering/ui/InspectorUI.h"
#include "glm/gtc/type_ptr.hpp"

namespace Engine::Components {
	void GizmoComponent::OnRemoved(Entity& entity)
	{
	}
	void GizmoComponent::OnAdded(Entity& entity)
	{
	}

	void GizmoComponent::RenderInspector(Entity& entity)
	{
		LeftLabelSliderFloat("Radius", &radius, 0.0, 10.0);
		LeftLabelColorEdit3("Color", glm::value_ptr(color));
	}

	void GizmoComponent::AddBindings()
	{
	}
} // namespace Engine::Components