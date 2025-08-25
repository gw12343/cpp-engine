//
// Created by gabe on 8/18/25.
//

#ifndef CPP_ENGINE_PARTICLE_H
#define CPP_ENGINE_PARTICLE_H

#include <Effekseer/Effekseer.h>
#include <EffekseerRendererGL/EffekseerRendererGL.h>

namespace Engine {
	class Particle {
	  public:
		Particle() = default; // empty constructor

		bool LoadFromFile(const std::string& path);

		[[nodiscard]] [[maybe_unused]] Effekseer::RefPtr<Effekseer::Effect> GetEffect() const { return effect_; }
		[[nodiscard]] [[maybe_unused]] const std::string&                   GetPath() const { return path_; }
		[[nodiscard]] bool                                                  IsValid() const { return effect_ != nullptr; }

	  private:
		Effekseer::RefPtr<Effekseer::Effect> effect_;
		std::string                          path_;
	};
} // namespace Engine

#endif // CPP_ENGINE_PARTICLE_H
