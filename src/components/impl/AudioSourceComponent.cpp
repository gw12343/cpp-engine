//
// Created by gabe on 6/24/25.
//

#include "components/Components.h"
#include "AudioSourceComponent.h"


#include "core/Entity.h"
#include "utils/Utils.h"
#include "imgui.h"

#include "ozz/animation/runtime/track.h"
#include "rendering/particles/ParticleManager.h"
#include "rendering/ui/InspectorUI.h"
#include "animation/AnimationManager.h"
#include "scripting/ScriptManager.h"
#include "misc/cpp/imgui_stdlib.h"
#include "assets/AssetManager.h"

namespace Engine::Components {
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