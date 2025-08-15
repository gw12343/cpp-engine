//
// Created by gabe on 6/30/25.
//

#ifndef CPP_ENGINE_SHADOWCASTERCOMPONENT_H
#define CPP_ENGINE_SHADOWCASTERCOMPONENT_H

#include "components/Components.h"

#include <cereal/cereal.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/array.hpp>


namespace Engine::Components {
	class ShadowCaster : public Component {
	  public:
		ShadowCaster() = default;

		template <class Archive>
		void serialize(Archive&)
		{
			// Intentionally empty — optional<> presence already indicates existence
		}

		void OnAdded(Entity& entity) override;
		void RenderInspector(Entity& entity) override;
	};
} // namespace Engine::Components

#endif // CPP_ENGINE_SHADOWCASTERCOMPONENT_H
