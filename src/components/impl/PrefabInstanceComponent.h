#pragma once

#include "assets/AssetHandle.h"
#include "components/Components.h"

#include <cereal/cereal.hpp>

namespace Engine::Components {
	class PrefabInstance : public Component {
	  public:
		PrefabHandle prefab;

		PrefabInstance() = default;
		explicit PrefabInstance(const PrefabHandle& handle) : prefab(handle) {}

		template <class Archive>
		void serialize(Archive& ar)
		{
			ar(cereal::make_nvp("prefab", prefab));
		}

		void OnAdded(Entity& entity) override;
		void OnRemoved(Entity& entity) override;
		void RenderInspector(Entity& entity) override;

		static void AddBindings();
	};
} // namespace Engine::Components
