//
// Created by gabe on 8/24/25.
//

#include "AudioDebugWindow.h"
#include "core/EngineData.h"
#include "imgui.h"
#include "components/Components.h"
#include "components/impl/EntityMetadataComponent.h"
#include "components/impl/TransformComponent.h"
#include "components/impl/AudioSourceComponent.h"

namespace Engine {
	void DrawAudioDebugWindow()
	{
		ImGui::Begin("Audio Debug");
		auto      audioView = GetCurrentSceneRegistry().view<Components::EntityMetadata, Components::Transform, Components::AudioSource>();
		glm::vec3 cameraPos = GetCamera().GetPosition();

		for (auto [entity, metadata, transform, audio] : audioView.each()) {
			if (audio.source) {
				float distance = glm::distance(cameraPos, transform.position);
				ImGui::Text("Entity: %s", metadata.name.c_str());
				ImGui::Text("Distance: %.2f units", distance);

				// Add sliders to adjust audio parameters
				bool paramsChanged = false;

				paramsChanged |= ImGui::SliderFloat("Volume", &audio.volume, 0.0f, 1.0f);
				if (paramsChanged) {
					audio.source->SetGain(audio.volume);
				}

				paramsChanged = false;
				paramsChanged |= ImGui::SliderFloat("Reference Distance", &audio.referenceDistance, 0.1f, 20.0f);
				paramsChanged |= ImGui::SliderFloat("Max Distance", &audio.maxDistance, 10.0f, 100.0f);
				paramsChanged |= ImGui::SliderFloat("Rolloff Factor", &audio.rolloffFactor, 0.1f, 5.0f);

				if (paramsChanged) {
					audio.source->ConfigureAttenuation(audio.referenceDistance, audio.maxDistance, audio.rolloffFactor);
				}

				// Calculate linear attenuation for display (matching OpenAL's AL_LINEAR_DISTANCE_CLAMPED)
				float attenuation;

				if (distance <= audio.referenceDistance) {
					// Within reference distance - full volume
					attenuation = 1.0f;
				}
				else if (distance >= audio.maxDistance) {
					// Beyond max distance - silent
					attenuation = 0.0f;
				}
				else {
					// Linear interpolation between reference and max distance
					attenuation = 1.0f - ((distance - audio.referenceDistance) / (audio.maxDistance - audio.referenceDistance));

					// Apply rolloff factor
					attenuation = pow(attenuation, audio.rolloffFactor);
				}

				ImGui::Text("Estimated Attenuation: %.3f", attenuation);
				ImGui::Text("Estimated Volume: %.3f", audio.volume * attenuation);

				// Add a visual representation of the attenuation
				ImGui::Text("Attenuation:");
				ImGui::SameLine();
				ImGui::ProgressBar(attenuation, ImVec2(100, 10));

				ImGui::Separator();
			}
		}
		ImGui::End();
	}
} // namespace Engine