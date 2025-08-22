//
// Created by gabe on 6/24/25.
//

#include "Components.h"
#include "AllComponents.h"

namespace Engine::Components {

	void RegisterAllComponentBindings()
	{
		EntityMetadata::AddBindings();

#define X(type, name, fancy) type::AddBindings();
		COMPONENT_LIST
#undef X
	}
} // namespace Engine::Components