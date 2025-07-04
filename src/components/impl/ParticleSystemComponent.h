//
// Created by gabe on 6/30/25.
//

#ifndef CPP_ENGINE_PARTICLESYSTEMCOMPONENT_H
#define CPP_ENGINE_PARTICLESYSTEMCOMPONENT_H

#include "components/Components.h"

namespace Engine::Components {
	class ParticleSystem : public Component {
	  public:
		std::string                          effectPath;
		Effekseer::RefPtr<Effekseer::Effect> effect;
		bool                                 autoPlay = true;
		bool                                 looping  = false;

		ParticleSystem() = default;
		explicit ParticleSystem(const std::string& path) : effectPath(path) {}
		void OnAdded(Entity& entity) override;
		void RenderInspector(Entity& entity) override;


		Effekseer::Handle handle = -1;
	};
} // namespace Engine::Components

#endif // CPP_ENGINE_PARTICLESYSTEMCOMPONENT_H
