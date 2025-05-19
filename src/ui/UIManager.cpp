#include "UIManager.h"

#include "components/Components.h"
#include "core/Engine.h"
#include "spdlog/spdlog.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Engine {

	UIManager::UIManager(GEngine* engine) : m_engine(engine)
	{
	}

	void UIManager::Initialize()
	{
		// Any initialization code for UI manager
	}

	void UIManager::Render()
	{
		RenderHierarchyWindow();
		RenderInspectorWindow();
		RenderAnimationWindow();
		RenderAudioDebugUI();

		// Display pause overlay when physics is disabled
		if (!m_engine->IsPhysicsEnabled()) {
			RenderPauseOverlay();
		}
	}

	void UIManager::RenderHierarchyWindow()
	{
		ImGui::Begin("Hierarchy");

		// Get all entities with EntityMetadata component
		auto view = m_engine->GetRegistry().view<Components::EntityMetadata>();

		for (auto entity : view) {
			Entity e(entity, m_engine);
			auto&  metadata = e.GetComponent<Components::EntityMetadata>();

			// Create a selectable item for each entity
			bool isSelected = (m_selectedEntity == e);
			// make the title the name, then the id after octothorpes
			std::string title = metadata.name + "##" + std::to_string((int) entity);
			if (ImGui::Selectable(title.c_str(), isSelected)) {
				m_selectedEntity = e;
			}
		}

		ImGui::End();
	}

	void UIManager::RenderInspectorWindow()
	{
		ImGui::Begin("Inspector");

		if (m_selectedEntity) {
			// Display entity name at the top
			auto& metadata = m_selectedEntity.GetComponent<Components::EntityMetadata>();
			ImGui::Text("Selected: %s", metadata.name.c_str());
			ImGui::Separator();

			// Render each component in the inspector
			if (m_selectedEntity.HasComponent<Components::EntityMetadata>()) {
				if (ImGui::CollapsingHeader("Entity Metadata", ImGuiTreeNodeFlags_DefaultOpen)) {
					m_selectedEntity.GetComponent<Components::EntityMetadata>().RenderInspector(m_selectedEntity);
				}
			}

			if (m_selectedEntity.HasComponent<Components::Transform>()) {
				if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
					m_selectedEntity.GetComponent<Components::Transform>().RenderInspector(m_selectedEntity);
				}
			}

			if (m_selectedEntity.HasComponent<Components::ModelRenderer>()) {
				if (ImGui::CollapsingHeader("Model Renderer")) {
					m_selectedEntity.GetComponent<Components::ModelRenderer>().RenderInspector(m_selectedEntity);
				}
			}

			if (m_selectedEntity.HasComponent<Components::RigidBodyComponent>()) {
				if (ImGui::CollapsingHeader("Rigid Body")) {
					m_selectedEntity.GetComponent<Components::RigidBodyComponent>().RenderInspector(m_selectedEntity);
				}
			}

			if (m_selectedEntity.HasComponent<Components::AudioSource>()) {
				if (ImGui::CollapsingHeader("Audio Source")) {
					auto& audioSource = m_selectedEntity.GetComponent<Components::AudioSource>();
					audioSource.RenderInspector(m_selectedEntity);

					// Handle the Play button functionality here since we have access to the sound manager
					if (!audioSource.isPlaying && ImGui::Button("Play")) {
						if (audioSource.source && !audioSource.soundName.empty()) {
							audioSource.Play(m_engine->GetSoundManager());
							spdlog::error("Playing sound");
						}
					}
				}
			}

			if (m_selectedEntity.HasComponent<Components::SkeletonComponent>()) {
				if (ImGui::CollapsingHeader("Skeleton")) {
					m_selectedEntity.GetComponent<Components::SkeletonComponent>().RenderInspector(m_selectedEntity);
				}
			}

			if (m_selectedEntity.HasComponent<Components::AnimationComponent>()) {
				if (ImGui::CollapsingHeader("Animation")) {
					m_selectedEntity.GetComponent<Components::AnimationComponent>().RenderInspector(m_selectedEntity);
				}
			}

			if (m_selectedEntity.HasComponent<Components::AnimationPoseComponent>()) {
				if (ImGui::CollapsingHeader("Animation Pose")) {
					m_selectedEntity.GetComponent<Components::AnimationPoseComponent>().RenderInspector(m_selectedEntity);
				}
			}

			if (m_selectedEntity.HasComponent<Components::AnimationWorkerComponent>()) {
				if (ImGui::CollapsingHeader("Animation Context")) {
					m_selectedEntity.GetComponent<Components::AnimationWorkerComponent>().RenderInspector(m_selectedEntity);
				}
			}

			if (m_selectedEntity.HasComponent<Components::SkinnedMeshComponent>()) {
				if (ImGui::CollapsingHeader("Skinned Mesh")) {
					m_selectedEntity.GetComponent<Components::SkinnedMeshComponent>().RenderInspector(m_selectedEntity);
				}
			}
		}
		else {
			ImGui::Text("No entity selected");
		}

		ImGui::End();
	}

	void UIManager::RenderAnimationWindow()
	{
		ImGui::Begin("Animation");

		if (ImGui::CollapsingHeader("Animation")) {
			auto& animManager = m_engine->GetAnimationManager();
			// auto& skeleton    = animManager.GetSkeleton();
			// auto& meshes      = animManager.GetMeshes();

			// ImGui::LabelText("Animated Joints", "%d animated joints", skeleton.num_joints());

			// int influences = 0;
			// for (const auto& mesh : meshes) {
			// 	influences = ozz::math::Max(influences, mesh.max_influences_count());
			// }
			// ImGui::LabelText("Influences", "%d influences (max)", influences);

			// int vertices = 0;
			// for (const auto& mesh : meshes) {
			// 	vertices += mesh.vertex_count();
			// }

			// ImGui::LabelText("Vertices", "%.1fK vertices", vertices / 1000.f);

			// int indices = 0;
			// for (const auto& mesh : meshes) {
			// 	indices += mesh.triangle_index_count();
			// }

			// ImGui::LabelText("Triangles", "%.1fK triangles", indices / 3000.f);
		}

		if (ImGui::CollapsingHeader("Rendering Options")) {
			auto& animManager    = m_engine->GetAnimationManager();
			auto& draw_skeleton  = animManager.GetDrawSkeleton();
			auto& draw_mesh      = animManager.GetDrawMesh();
			auto& render_options = animManager.GetRenderOptions();

			ImGui::Checkbox("Show Skeleton", &draw_skeleton);
			ImGui::Checkbox("Show Mesh", &draw_mesh);
			ImGui::Separator();

			ImGui::Checkbox("Show triangles", &render_options.triangles);
			ImGui::Checkbox("Show texture", &render_options.texture);
			ImGui::Checkbox("Show vertices", &render_options.vertices);
			ImGui::Checkbox("Show normals", &render_options.normals);
			ImGui::Checkbox("Show tangents", &render_options.tangents);
			ImGui::Checkbox("Show binormals", &render_options.binormals);
			ImGui::Checkbox("Show colors", &render_options.colors);
			ImGui::Checkbox("Wireframe", &render_options.wireframe);
			ImGui::Checkbox("Skip skinning", &render_options.skip_skinning);
		}

		ImGui::End();
	}

	void UIManager::RenderAudioDebugUI()
	{
		ImGui::Begin("Audio Debug");
		auto      audioView = m_engine->GetRegistry().view<Components::EntityMetadata, Components::Transform, Components::AudioSource>();
		glm::vec3 cameraPos = m_engine->GetCamera().GetPosition();

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
				float attenuation = 1.0f;

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

	void UIManager::RenderPauseOverlay()
	{
		ImGuiIO& io = ImGui::GetIO();
		ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 10, 10), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
		ImGui::SetNextWindowBgAlpha(0.35f);
		ImGui::Begin("PauseOverlay",
		             nullptr,
		             ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
		                 ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove);
		ImGui::Text("PHYSICS PAUSED");
		ImGui::Text("Press P to resume");
		ImGui::End();
	}

} // namespace Engine
