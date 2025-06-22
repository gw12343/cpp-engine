#include "components/Components.h"

#include "core/Engine.h"
#include "core/Entity.h"
#include "utils/Utils.h"

#include <codecvt>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <locale>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/animation/runtime/track.h>
#include <ozz/base/containers/vector.h>
#include <ozz/base/maths/simd_math.h>
#include <string>
namespace Engine {

	namespace Components {

		void EntityMetadata::OnAdded(Entity& entity)
		{
		}

		void Transform::OnAdded(Entity& entity)
		{
		}

		void ModelRenderer::OnAdded(Entity& entity)
		{
		}

		void RigidBodyComponent::OnAdded(Entity& entity)
		{
		}

		void AudioSource::OnAdded(Entity& entity)
		{
			ENGINE_ASSERT(entity.m_engine, "AudioSource::OnAdded: Entity's engine pointer is null");
			// If autoPlay is enabled, try to play the sound
			if (autoPlay && !soundName.empty()) {
				// Get the sound manager from the engine
				auto& soundManager = entity.m_engine->GetSoundManager();
				Play(soundManager);
			}
		}

		void EntityMetadata::RenderInspector(Entity& entity)
		{
			char nameBuffer[256];
			strcpy(nameBuffer, name.c_str());
			if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer))) {
				name = nameBuffer;
			}

			char tagBuffer[256];
			strcpy(tagBuffer, tag.c_str());
			if (ImGui::InputText("Tag", tagBuffer, sizeof(tagBuffer))) {
				tag = tagBuffer;
			}

			ImGui::Checkbox("Active", &active);
		}

		void Transform::RenderInspector(Entity& entity)
		{
			bool updatePhysicsPositionManually = false;
			if (ImGui::DragFloat3("Position", glm::value_ptr(position), 0.1f)) {
				updatePhysicsPositionManually = true;
			}

			glm::vec3 eulerAngles = GetEulerAngles();
			if (ImGui::DragFloat3("Rotation", glm::value_ptr(eulerAngles), 1.0f)) {
				SetRotation(eulerAngles);
				updatePhysicsPositionManually = true;
			}

			if (updatePhysicsPositionManually) {
				if (entity.HasComponent<RigidBodyComponent>()) {
					auto& rb = entity.GetComponent<RigidBodyComponent>();
					ENGINE_VERIFY(rb.physicsSystem, "Transform::RenderInspector: RigidBodyComponent has null physicsSystem");
					BodyInterface& body_interface = rb.physicsSystem->GetBodyInterface();
					// convert pos and rot to jolt types
					RVec3 joltPos = RVec3(position.x, position.y, position.z);
					Quat  joltRot = Quat::sEulerAngles(RVec3(glm::radians(eulerAngles.x), glm::radians(eulerAngles.y), glm::radians(eulerAngles.z)));
					body_interface.SetPositionAndRotation(rb.bodyID, joltPos, joltRot, EActivation::Activate);
				}
			}

			ImGui::DragFloat3("Scale", glm::value_ptr(scale), 0.1f);
		}

		void ModelRenderer::RenderInspector(Entity& entity)
		{
			ImGui::Checkbox("Visible", &visible);

			if (model) {
				ImGui::Text("Model: Loaded");
				// Could add more model info here
			}
			else {
				ImGui::Text("Model: None");
			}
		}

		void RigidBodyComponent::RenderInspector(Entity& entity)
		{
			ImGui::Text("Body ID: %u", bodyID.GetIndex());
			// Could add more physics properties here
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

		void SkeletonComponent::OnAdded(Entity& entity)
		{
			ENGINE_ASSERT(entity.m_engine, "SkeletonComponent::OnAdded: Entity's engine pointer is null");

			if (!skeletonPath.empty()) {
				skeleton = entity.m_engine->GetAnimationManager().LoadSkeletonFromPath(skeletonPath);
				if (!skeleton) {
					spdlog::error("Failed to load skeleton from path: {}", skeletonPath);
				}
				else {
					SPDLOG_INFO("Loaded skeleton from path: {}", skeletonPath);
				}
			}
		}


		void SkeletonComponent::RenderInspector(Entity& entity)
		{
			ImGui::Text("Skeleton: %s", skeleton ? "Loaded" : "Null");
			ImGui::Text("Joints: %d", skeleton ? skeleton->num_joints() : 0);
			ImGui::Text("SOA Joints: %d", skeleton ? skeleton->num_soa_joints() : 0);
		}

		void AnimationComponent::OnAdded(Entity& entity)
		{
			ENGINE_ASSERT(entity.m_engine, "AnimationComponent::OnAdded: Entity's engine pointer is null");

			if (!animationPath.empty()) {
				animation = entity.m_engine->GetAnimationManager().LoadAnimationFromPath(animationPath);
				if (!animation) {
					spdlog::error("Failed to load animation from path: {}", animationPath);
				}
				else {
					SPDLOG_INFO("Loaded animation from path: {}", animationPath);
				}
			}
		}

		void AnimationComponent::RenderInspector(Entity& entity)
		{
			ImGui::Text("Animation: %s", animation ? "Loaded" : "Null");
			ImGui::Text("Tracks: %d", animation ? animation->num_tracks() : 0);
			ImGui::Text("Duration: %.2f", animation ? animation->duration() : 0.0f);
		}

		void AnimationPoseComponent::OnAdded(Entity& entity)
		{
			ENGINE_VERIFY(entity.HasComponent<SkeletonComponent>(), "AnimationPoseComponent::OnAdded: Missing SkeletonComponent");
			auto& skeletonComponent = entity.GetComponent<SkeletonComponent>();
			ENGINE_VERIFY(skeletonComponent.skeleton, "AnimationPoseComponent::OnAdded: SkeletonComponent has null skeleton");
			ENGINE_ASSERT(entity.m_engine, "AnimationPoseComponent::OnAdded: Entity's engine pointer is null");


			// Allocate pose data
			local_pose = AnimationManager::AllocateLocalPose(skeletonComponent.skeleton);
			model_pose = AnimationManager::AllocateModelPose(skeletonComponent.skeleton);

			if (!local_pose || !model_pose) {
				spdlog::error("Failed to allocate pose data for entity");
			}
			else {
				SPDLOG_INFO("Allocated pose data for entity");
			}
		}

		void AnimationPoseComponent::RenderInspector(Entity& entity)
		{
			ImGui::Text("Local Pose: %s", local_pose ? std::to_string(local_pose->size()).c_str() : "Null");
			ImGui::Text("Model Pose: %s", model_pose ? std::to_string(model_pose->size()).c_str() : "Null");
		}


		std::unordered_set<ozz::animation::SamplingJob::Context*> AnimationWorkerComponent::s_contexts;

		void AnimationWorkerComponent::CleanAnimationContexts()
		{
			for (ozz::animation::SamplingJob::Context* ctx : s_contexts) {
				delete ctx;
			}
		}

		void AnimationWorkerComponent::OnAdded(Entity& entity)
		{
			context = new ozz::animation::SamplingJob::Context();
			s_contexts.insert(context);
			// Get the animation component, then resize context for correct number of tracks
			ENGINE_ASSERT(context, "AnimationWorkerComponent::OnAdded: Failed to allocate SamplingJob::Context");

			ENGINE_VERIFY(entity.HasComponent<AnimationComponent>(), "AnimationWorkerComponent::OnAdded: Missing AnimationComponent");
			auto& animationComponent = entity.GetComponent<AnimationComponent>();
			ENGINE_VERIFY(animationComponent.animation, "AnimationWorkerComponent::OnAdded: AnimationComponent has null animation");

			context->Resize(animationComponent.animation->num_tracks());
			SPDLOG_INFO("Resized context for {} tracks", animationComponent.animation->num_tracks());
		}

		void AnimationWorkerComponent::RenderInspector(Entity& entity)
		{
			ImGui::Text("Sampling Job Context: %s", context ? "Initialized" : "Null");
		}


		std::unordered_set<std::vector<ozz::math::Float4x4>*> SkinnedMeshComponent::s_skin_mats;
		std::unordered_set<ozz::vector<Engine::Mesh>*>        SkinnedMeshComponent::s_all_meshes;

		void SkinnedMeshComponent::CleanSkinnedModels()
		{
			for (std::vector<ozz::math::Float4x4>* mat : s_skin_mats) {
				delete mat;
			}

			for (ozz::vector<Engine::Mesh>* mesh : s_all_meshes) {
				delete mesh;
			}
		}

		void SkinnedMeshComponent::OnAdded(Entity& entity)
		{
			if (!meshPath.empty()) {
				meshes = AnimationManager::LoadMeshesFromPath(meshPath);
				s_all_meshes.insert(meshes);
				if (!meshes) {
					SPDLOG_ERROR("Failed to load meshes from path: {}", meshPath);
				}
				else {
					SPDLOG_INFO("Loaded SKINNED MESHES from path: {}", meshPath);
				}
			}
			ENGINE_ASSERT(entity.m_engine, "SkinnedMeshComponent::OnAdded: Entity's engine pointer is null");
			if (!meshes) {
				ENGINE_VERIFY(false, "SkinnedMeshComponent::OnAdded: Failed to load meshes");
				return;
			}


			skinning_matrices = new std::vector<ozz::math::Float4x4>();
			s_skin_mats.insert(skinning_matrices);
			// Computes the number of skinning matrices required to skin all meshes
			// A mesh is skinned by only a subset of joints, so the number of skinning
			// matrices might be less that the number of skeleton joints
			// Mesh::joint_remaps is used to know how to order skinning matrices. So
			// the number of matrices required is the size of joint_remaps
			size_t num_skinning_matrices = 0;
			for (const Engine::Mesh& mesh : *meshes) {
				num_skinning_matrices = ozz::math::Max(num_skinning_matrices, mesh.joint_remaps.size());
			}

			// Allocates skinning matrices
			skinning_matrices->resize(num_skinning_matrices);
			ENGINE_ASSERT(skinning_matrices, "SkinnedMeshComponent::OnAdded: Failed to allocate skinning matrices");
		}

		void SkinnedMeshComponent::RenderInspector(Entity& entity)
		{
			ImGui::Text("Meshes: %s", meshes ? std::to_string(meshes->size()).c_str() : "Null");
			ImGui::Text("Skinning Matrices: %s", skinning_matrices ? std::to_string(skinning_matrices->size()).c_str() : "Null");
		}


		void ParticleSystem::OnAdded(Entity& entity)
		{
			if (!effect) {
				// Convert path string
				ENGINE_ASSERT(!effectPath.empty(), "ParticleSystem::OnAdded: effectPath is empty");
				std::u16string  utf16 = std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>{}.from_bytes(effectPath);
				const char16_t* raw   = utf16.c_str();

				ENGINE_ASSERT(entity.m_engine, "ParticleSystem::OnAdded: Entity's engine pointer is null");
				const auto& manager = entity.m_engine->GetParticleManager().GetManager();
				ENGINE_VERIFY(manager != nullptr, "ParticleSystem::OnAdded: Failed to get Effekseer manager");
				// Spawn effect
				effect = Effekseer::Effect::Create(manager, raw);
				ENGINE_VERIFY(effect != nullptr, "ParticleSystem::OnAdded: Failed to create Effekseer effect");
				// Get initial transform
				auto transform = entity.GetComponent<Components::Transform>();
				// Spawn particle
				handle = manager->Play(effect, transform.position.x, transform.position.y, transform.position.z);
			}
		}

		void ParticleSystem::RenderInspector(Entity& entity)
		{
			ENGINE_ASSERT(entity.m_engine, "ParticleSystem::RenderInspector: Entity's engine pointer is null");

			ImGui::Text("Handle: %d", handle);
			const auto& manager = entity.m_engine->GetParticleManager().GetManager();
			ENGINE_VERIFY(manager != nullptr, "ParticleSystem::RenderInspector: Failed to get Effekseer manager");
			bool paused = manager->GetPaused(handle);

			if (ImGui::Button(paused ? "Unpause" : "Pause")) {
				manager->SetPaused(handle, !paused);
			}

			if (ImGui::Button("Restart")) {
				manager->StopEffect(handle);
				auto transform = entity.GetComponent<Components::Transform>();
				handle         = manager->Play(effect, transform.position.x, transform.position.y, transform.position.z);
			}
		}
	} // namespace Components
} // namespace Engine
