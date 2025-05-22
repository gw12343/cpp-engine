#include "components/Components.h"

#include "core/Engine.h"
#include "core/Entity.h"

#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/blending_job.h>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/animation/runtime/track.h>
#include <ozz/animation/runtime/track_sampling_job.h>
#include <ozz/animation/runtime/track_triggering_job.h>
#include <ozz/base/containers/vector.h>
#include <ozz/base/maths/simd_math.h>
#include <ozz/base/maths/soa_transform.h>
#include <ozz/base/maths/vec_float.h>
#include <ozz/base/memory/unique_ptr.h>

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
					auto&          rb             = entity.GetComponent<RigidBodyComponent>();
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
			// Check if the entity has a skeleton component
			if (!entity.HasComponent<SkeletonComponent>()) {
				spdlog::error("AnimationPoseComponent requires a SkeletonComponent");
				return;
			}
			auto& skeletonComponent = entity.GetComponent<SkeletonComponent>();
			if (!skeletonComponent.skeleton) {
				spdlog::error("AnimationPoseComponent requires a valid SkeletonComponent with a valid skeleton!");
				return;
			}


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
			if (entity.HasComponent<AnimationComponent>()) {
				auto& animationComponent = entity.GetComponent<AnimationComponent>();
				if (animationComponent.animation) {
					context->Resize(animationComponent.animation->num_tracks());
					SPDLOG_INFO("Resized context for {} tracks", animationComponent.animation->num_tracks());
				}
				else {
					// fix: if the animation is loaded at a later time, we need to resize the context when the animation is loaded
					spdlog::error("AnimationWorkerComponent requires a valid AnimationComponent with a valid animation!");
				}
			}
			else {
				spdlog::error("AnimationWorkerComponent requires an AnimationComponent");
			}
		}

		void AnimationWorkerComponent::RenderInspector(Entity& entity)
		{
			ImGui::Text("Sampling Job Context: %s", context ? "Initialized" : "Null");
		}


		std::unordered_set<std::vector<ozz::math::Float4x4>*> SkinnedMeshComponent::s_skin_mats;
		std::unordered_set<ozz::vector<myns::Mesh>*>          SkinnedMeshComponent::s_all_meshes;

		void SkinnedMeshComponent::CleanSkinnedModels()
		{
			for (std::vector<ozz::math::Float4x4>* mat : s_skin_mats) {
				delete mat;
			}

			for (ozz::vector<myns::Mesh>* mesh : s_all_meshes) {
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

			if (!meshes) {
				spdlog::error("SkinnedMeshComponent requires a valid mesh!");
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
			for (const myns::Mesh& mesh : *meshes) {
				num_skinning_matrices = ozz::math::Max(num_skinning_matrices, mesh.joint_remaps.size());
			}

			// Allocates skinning matrices
			skinning_matrices->resize(num_skinning_matrices);
		}

		void SkinnedMeshComponent::RenderInspector(Entity& entity)
		{
			ImGui::Text("Meshes: %s", meshes ? std::to_string(meshes->size()).c_str() : "Null");
			ImGui::Text("Skinning Matrices: %s", skinning_matrices ? std::to_string(skinning_matrices->size()).c_str() : "Null");
		}
	} // namespace Components
} // namespace Engine
