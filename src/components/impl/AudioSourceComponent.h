//
// Created by gabe on 6/30/25.
//

#ifndef CPP_ENGINE_AUDIOSOURCECOMPONENT_H
#define CPP_ENGINE_AUDIOSOURCECOMPONENT_H

#include "components/Components.h"

namespace Engine::Components {
	class AudioSource : public Component {
	  public:
		std::string soundName;
		bool        autoPlay  = false;
		bool        looping   = false;
		float       volume    = 1.0f;
		float       pitch     = 1.0f;
		bool        isPlaying = false;

		// Attenuation parameters
		float referenceDistance = 1.0f;   // Distance at which volume is at maximum
		float maxDistance       = 100.0f; // Distance at which attenuation stops
		float rolloffFactor     = 1.0f;   // How quickly the sound attenuates

		std::shared_ptr<Audio::SoundSource> source;

		AudioSource() = default;

		explicit AudioSource(std::string name, bool loop = false, float vol = 1.0f, float p = 1.0f, bool play = false, float refDist = 1.0f, float maxDist = 100.0f, float rolloff = 1.0f)
		    : soundName(std::move(name)), looping(loop), volume(vol), pitch(p), autoPlay(play), referenceDistance(refDist), maxDistance(maxDist), rolloffFactor(rolloff)
		{
			source = std::make_shared<Audio::SoundSource>(looping);
			source->SetGain(volume);
			source->SetPitch(pitch);

			// Explicitly configure attenuation parameters
			source->ConfigureAttenuation(referenceDistance, maxDistance, rolloffFactor);

			// Log the configuration
			SPDLOG_INFO("Created AudioSource with attenuation: ref={}, max={}, rolloff={}", referenceDistance, maxDistance, rolloffFactor);
		}

		void Play(Audio::SoundManager& soundManager)
		{
			if (!source) return;

			auto buffer = soundManager.GetSound(soundName);
			if (buffer) {
				source->Play(*buffer);
				isPlaying = true;
			}
		}

		void Stop()
		{
			if (source) {
				source->Stop();
				isPlaying = false;
			}
		}

		void OnAdded(Entity& entity) override;
		void RenderInspector(Entity& entity) override;
	};
} // namespace Engine::Components

#endif // CPP_ENGINE_AUDIOSOURCECOMPONENT_H
