//
// Created by gabe on 6/24/25.
//

#include "AudioSourceComponent.h"

#include "core/EngineData.h"
#include "core/Entity.h"
#include "rendering/ui/InspectorUI.h"
#include "scripting/ScriptManager.h"
#include "sound/SoundManager.h"

namespace Engine::Components {
	AudioSource::AudioSource(SoundHandle buf, bool loop, float vol, float p, bool play, float refDist, float maxDist, float rolloff)
	    : autoPlay(play), looping(loop), volume(vol), pitch(p), referenceDistance(refDist), maxDistance(maxDist), rolloffFactor(rolloff), buffer(buf)
	{
		source = std::make_shared<Audio::SoundSource>(looping);
		source->SetGain(volume);
		source->SetPitch(pitch);
		source->ConfigureAttenuation(referenceDistance, maxDistance, rolloffFactor);
		GetDefaultLogger()->info("Created AudioSource with attenuation: ref={}, max={}, rolloff={}", referenceDistance, maxDistance, rolloffFactor);
	}

	void AudioSource::Play()
	{
		if (!source) return;
		source->Play(buffer);
		isPlaying = true;
	}

	void AudioSource::Stop()
	{
		if (source) {
			source->Stop();
			isPlaying = false;
		}
	}

	void AudioSource::OnRemoved(Entity& entity)
	{
	}

	void AudioSource::OnAdded(Entity& entity)
	{
		source = std::make_shared<Audio::SoundSource>(looping);
	}

	void AudioSource::RenderInspector(Entity& entity)
	{
		LeftLabelAssetSound("Sound", &buffer);

		LeftLabelCheckbox("Auto Play", &autoPlay);
		LeftLabelCheckbox("Looping", &looping);


		LeftLabelSliderFloat("Volume", &volume, 0.0f, 1.0f);
		LeftLabelSliderFloat("Pitch", &pitch, 0.5f, 2.0f);

		ImGui::Separator();
		ImGui::Text("Attenuation Settings:");
		LeftLabelSliderFloat("Reference Distance", &referenceDistance, 0.1f, 20.0f);
		LeftLabelSliderFloat("Max Distance", &maxDistance, 1.0f, 200.0f);
		LeftLabelSliderFloat("Rolloff Factor", &rolloffFactor, 0.1f, 5.0f);

		ImGui::Separator();
		if (isPlaying) {
			if (ImGui::Button("Stop")) {
				Stop();
			}
		}
		else if (ImGui::Button("Play")) {
			Play();
		}
	}


	void AudioSource::AddBindings()
	{
		auto& lua = GetScriptManager().lua;

		lua.new_usertype<AudioSource>("AudioSource",
		                              "autoPlay",
		                              &AudioSource::autoPlay,
		                              "looping",
		                              &AudioSource::looping,
		                              "volume",
		                              &AudioSource::volume,
		                              "pitch",
		                              &AudioSource::pitch,
		                              "isPlaying",
		                              &AudioSource::isPlaying,
		                              "referenceDistance",
		                              &AudioSource::referenceDistance,
		                              "maxDistance",
		                              &AudioSource::maxDistance,
		                              "rolloffFactor",
		                              &AudioSource::rolloffFactor,


		                              "play",
		                              &AudioSource::Play,
		                              "stop",
		                              &AudioSource::Stop,

		                              "setSound",
		                              &AudioSource::SetSound

		);
	}


} // namespace Engine::Components

// #include "assets/AssetManager.inl"