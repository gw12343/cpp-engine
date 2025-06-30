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

namespace Engine::Components {
	void AudioSource::OnAdded(Entity& entity)
	{
		// If autoPlay is enabled, try to play the sound
		if (autoPlay && !soundName.empty()) {
			// Get the sound manager from the engine
			auto& soundManager = GetSoundManager();
			Play(soundManager);
		}
	}

	void AudioSource::RenderInspector(Entity& entity)
	{
		char soundNameBuffer[256];
		strcpy(soundNameBuffer, soundName.c_str());
		if (ImGui::InputText("Sound", soundNameBuffer, sizeof(soundNameBuffer))) {
			soundName = soundNameBuffer;
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