//
// Created by gabe on 6/30/25.
//

#ifndef CPP_ENGINE_PARTICLESYSTEMCOMPONENT_H
#define CPP_ENGINE_PARTICLESYSTEMCOMPONENT_H

#include "components/Components.h"
#include "rendering/particles/Particle.h"

#include <cereal/cereal.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/array.hpp>
#include <utility>


namespace Engine::Components {
	class ParticleSystem : public Component {
	  public:
		AssetHandle<Particle> effect;
		bool                  autoPlay = true;
		bool                  looping  = false;

		ParticleSystem() = default;
		explicit ParticleSystem(AssetHandle<Particle> particle) : effect(std::move(particle)) {}

		template <class Archive>
		void serialize(Archive& ar)
		{
			ar(cereal::make_nvp("effect", effect), cereal::make_nvp("autoPlay", autoPlay), cereal::make_nvp("looping", looping));
		}

		void OnAdded(Entity& entity) override;
		void OnRemoved(Entity& entity) override;
		void RenderInspector(Entity& entity) override;

        void Play(Entity& entity);


		Effekseer::Handle handle = -1;
	};
} // namespace Engine::Components

#endif // CPP_ENGINE_PARTICLESYSTEMCOMPONENT_H
