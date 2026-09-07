//
// Created by gabe on 6/30/25.
//

#ifndef CPP_ENGINE_AUDIOSOURCECOMPONENT_H
#define CPP_ENGINE_AUDIOSOURCECOMPONENT_H

#include "assets/AssetHandle.h"
#include "components/Components.h"

#include <cereal/cereal.hpp>
#include <memory>

namespace Engine::Audio {
	class SoundSource;
}

namespace Engine::Components {
	class AudioSource : public Component {
	  public:
		bool  autoPlay  = false;
		bool  looping   = false;
		float volume    = 1.0f;
		float pitch     = 1.0f;
		bool  isPlaying = false;

		float referenceDistance = 1.0f;
		float maxDistance       = 100.0f;
		float rolloffFactor     = 1.0f;

		std::shared_ptr<Audio::SoundSource> source;
		SoundHandle                         buffer;

		AudioSource() = default;

		template <class Archive>
		void serialize(Archive& ar)
		{
			ar(cereal::make_nvp("autoPlay", autoPlay),
			   cereal::make_nvp("looping", looping),
			   cereal::make_nvp("volume", volume),
			   cereal::make_nvp("pitch", pitch),
			   cereal::make_nvp("isPlaying", isPlaying),
			   cereal::make_nvp("referenceDistance", referenceDistance),
			   cereal::make_nvp("maxDistance", maxDistance),
			   cereal::make_nvp("rolloffFactor", rolloffFactor),
			   cereal::make_nvp("buffer", buffer));
		}

		explicit AudioSource(SoundHandle buf, bool loop = false, float vol = 1.0f, float p = 1.0f, bool play = false, float refDist = 1.0f, float maxDist = 100.0f, float rolloff = 1.0f);

		void Play();
		void Stop();

		void SetSound(SoundHandle sound) { buffer = sound; }

		void OnAdded(Entity& entity) override;
		void OnRemoved(Entity& entity) override;
		void RenderInspector(Entity& entity) override;

		static void AddBindings();
	};
} // namespace Engine::Components

#endif // CPP_ENGINE_AUDIOSOURCECOMPONENT_H
