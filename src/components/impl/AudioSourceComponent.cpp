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
		// If autoPlay is enabled, try to play the sound
		if (autoPlay && buffer.IsValid()) {
			// Get the sound manager from the engine
			auto& soundManager = GetSoundManager();
			Play(soundManager);
		}
	}

	void AudioSource::RenderInspector(Entity& entity)
	{
		std::string newID = buffer.GetID();
		if (ImGui::InputText("Sound", &newID)) {
			buffer = AssetHandle<Audio::SoundBuffer>(newID);
		}

		ImGui::Checkbox("Auto Play", &autoPlay);
		ImGui::Checkbox("Looping", &looping);

		ImGui::SliderFloat("Volume", &volume, 0.0f, 1.0f);
		ImGui::SliderFloat("Pitch", &pitch, 0.5f, 2.0f);

		ImGui::Separator();
		ImGui::Text("Attenuation Settings:");
		ImGui::SliderFloat("Reference Distance", &referenceDistance, 0.1f, 20.0f);
		ImGui::SliderFloat("Max Distance", &maxDistance, 1.0f, 200.0f);
		ImGui::SliderFloat("Rolloff Factor", &rolloffFactor, 0.1f, 5.0f);

		ImGui::Separator();
		if (isPlaying) {
			if (ImGui::Button("Stop")) {
				Stop();
			}
		}
	}


} // namespace Engine::Components

// #include "assets/AssetManager.inl"