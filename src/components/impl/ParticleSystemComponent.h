//
// Created by gabe on 6/30/25.
//

#ifndef CPP_ENGINE_PARTICLESYSTEMCOMPONENT_H
#define CPP_ENGINE_PARTICLESYSTEMCOMPONENT_H

#include "assets/AssetHandle.h"
#include "components/Components.h"

#include <cereal/cereal.hpp>
#include <glm/glm.hpp>
#include <utility>

namespace Engine::Components {
	class ParticleSystem : public Component {
	  public:
		ParticleHandle effect{};
		bool           autoPlay = true;
		bool           looping  = false;

		ParticleSystem() = default;
		explicit ParticleSystem(ParticleHandle particle) : effect(std::move(particle)) {}

		template <class Archive>
		void serialize(Archive& ar)
		{
			ar(cereal::make_nvp("effect", effect), cereal::make_nvp("autoPlay", autoPlay), cereal::make_nvp("looping", looping));
		}

		void OnAdded(Entity& entity) override;
		void OnRemoved(Entity& entity) override;
		void RenderInspector(Entity& entity) override;

		static void AddBindings();

		int handle = -1;

		glm::vec3 lastPos{0.f};
		bool      posValid = false;
	};
} // namespace Engine::Components

#endif // CPP_ENGINE_PARTICLESYSTEMCOMPONENT_H
