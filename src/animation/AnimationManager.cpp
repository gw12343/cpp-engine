#include "AnimationManager.h"

#include "AnimationUtils.h"
#include "core/Entity.h"

#include "components/impl/AnimationComponent.h"
#include "components/impl/TransformComponent.h"
#include "components/impl/AnimationPoseComponent.h"
#include "components/impl/AnimationWorkerComponent.h"
#include "components/impl/SkeletonComponent.h"
#include "components/impl/SkinnedMeshComponent.h"

#include <spdlog/spdlog.h>
#include <utils/Utils.h>
#include "core/SceneManager.h"
#include "assets/AssetManager.h"
namespace Engine {

	void AnimationManager::onInit()
	{
		draw_skeleton_ = false;
		draw_mesh_     = true;
		// Initialize render options with defaults
		render_options_.triangles     = true;
		render_options_.texture       = true;
		render_options_.vertices      = false;
		render_options_.normals       = false;
		render_options_.tangents      = false;
		render_options_.binormals     = false;
		render_options_.colors        = true;
		render_options_.wireframe     = false;
		render_options_.skip_skinning = false;


		renderer_ = ozz::make_unique<RendererImpl>();

		bool success = renderer_->Initialize();
		if (!success) {
			log->error("Failed to initialize animation renderer");
			return;
		}
		else {
			log->info("Initialized animated renderer");
		}
	}


	void AnimationManager::onShutdown()
	{
		// Clean up loaded skeletons and animations
		for (auto& pair : loaded_skeletons_) {
			pair.second.reset();
		}
		for (auto& pair : loaded_animations_) {
			pair.second.reset();
		}

		renderer_.reset();
	}


	void AnimationManager::onUpdate(float deltaTime)
	{
		// Get all entities with a AnimationWorkerComponent and AnimationComponent and AnimationPoseComponent and SkeletonComponent
		auto view = GetCurrentSceneRegistry().view<Components::AnimationWorkerComponent, Components::AnimationComponent>();
		for (auto entity : view) {
			Entity e(entity, GetCurrentScene());
			auto&  animationWorkerComponent = e.GetComponent<Components::AnimationWorkerComponent>();
			auto&  animationComponent       = e.GetComponent<Components::AnimationComponent>();
			auto&  animationPoseComponent   = e.GetComponent<Components::AnimationPoseComponent>();
			auto&  skeletonComponent        = e.GetComponent<Components::SkeletonComponent>();

			// Update animation time
			controller_.set_time_ratio((float) (sin(glfwGetTime() / 3.0f) / 2.0 + 0.5f));

			// Samples optimized animation at t = animation_time_
			ozz::animation::SamplingJob sampling_job;
			sampling_job.animation = animationComponent.animation;
			sampling_job.context   = animationWorkerComponent.context;
			sampling_job.ratio     = controller_.time_ratio();
			sampling_job.output    = ozz::make_span(*animationPoseComponent.local_pose);
			if (!sampling_job.Run()) {
				log->error("Failed to sample animation");
				return;
			}

			ozz::animation::LocalToModelJob ltm_job;
			ltm_job.skeleton = skeletonComponent.skeleton;
			ltm_job.input    = ozz::make_span(*animationPoseComponent.local_pose);
			ltm_job.output   = ozz::make_span(*animationPoseComponent.model_pose);
			if (!ltm_job.Run()) {
				log->error("Failed to convert to model space");
				return;
			}
		}
	}


	void AnimationManager::Render()
	{
		// Get all entities with a SkinnedMeshComponent and transform and AnimationPoseComponent
		auto view = GetCurrentSceneRegistry().view<Components::SkinnedMeshComponent, Components::Transform>();
		for (auto entity : view) {
			Entity                    e(entity, GetCurrentScene());
			auto&                     skinnedMeshComponent   = e.GetComponent<Components::SkinnedMeshComponent>();
			auto&                     animationPoseComponent = e.GetComponent<Components::AnimationPoseComponent>();
			const ozz::math::Float4x4 transform              = FromMatrix(e.GetComponent<Components::Transform>().GetMatrix());

			// Render each mesh
			for (const Engine::Mesh& mesh : *skinnedMeshComponent.meshes) {
				// Render the mesh

				// Builds skinning matrices, based on the output of the animation stage
				// The mesh might not use (aka be skinned by) all skeleton joints. We
				// use the joint remapping table (available from the mesh object) to
				// reorder model-space matrices and build skinning ones
				for (size_t i = 0; i < mesh.joint_remaps.size(); ++i) {
					(*skinnedMeshComponent.skinning_matrices)[i] = (*animationPoseComponent.model_pose)[mesh.joint_remaps[i]] * mesh.inverse_bind_poses[i];
				}
				renderer_->DrawSkinnedMesh(mesh, ozz::make_span(*skinnedMeshComponent.skinning_matrices), transform, render_options_);
			}
		}

		// if (draw_skeleton_) {
		// 	success &= renderer_->DrawPosture(skeleton_, make_span(models_), transform, true);
		// }
	}

	ozz::animation::Skeleton* AnimationManager::LoadSkeletonFromPath(const std::string& path)
	{
		// Check if skeleton is already loaded
		auto it = loaded_skeletons_.find(path);
		if (it != loaded_skeletons_.end()) {
			return it->second.get();
		}

		// Create a new skeleton
		auto skeleton = std::make_unique<ozz::animation::Skeleton>();

		// Load the skeleton from file
		if (!LoadSkeleton(path.c_str(), skeleton.get())) {
			log->error("Failed to load skeleton from path: {}", path);
			return nullptr;
		}

		// Store the skeleton in the map and return a pointer to it
		ozz::animation::Skeleton* result = skeleton.get();
		loaded_skeletons_[path]          = std::move(skeleton);
		return result;
	}

	ozz::animation::Animation* AnimationManager::LoadAnimationFromPath(const std::string& path)
	{
		// Check if animation is already loaded
		auto it = loaded_animations_.find(path);
		if (it != loaded_animations_.end()) {
			return it->second.get();
		}

		// Create a new animation
		auto animation = std::make_unique<ozz::animation::Animation>();

		// Load the animation from file
		if (!LoadAnimation(path.c_str(), animation.get())) {
			log->error("Failed to load animation from path: {}", path);
			return nullptr;
		}

		// Store the animation in the map and return a pointer to it
		ozz::animation::Animation* result = animation.get();
		loaded_animations_[path]          = std::move(animation);
		return result;
	}

	std::vector<ozz::math::SoaTransform>* AnimationManager::AllocateLocalPose(const ozz::animation::Skeleton* skeleton)
	{
		if (!skeleton) {
			GetAnimationManager().log->error("Cannot allocate local pose for null skeleton");
			return nullptr;
		}

		// Create a new vector of SoaTransforms with the correct size
		auto local_pose = new std::vector<ozz::math::SoaTransform>();
		local_pose->resize(skeleton->num_soa_joints());

		GetAnimationManager().log->info("Allocated local pose with {} SoA joints", skeleton->num_soa_joints());
		return local_pose;
	}

	std::vector<ozz::math::Float4x4>* AnimationManager::AllocateModelPose(const ozz::animation::Skeleton* skeleton)
	{
		if (!skeleton) {
			GetAnimationManager().log->error("Cannot allocate model pose for null skeleton");
			return nullptr;
		}

		// Create a new vector of Float4x4 matrices with the correct size
		auto model_pose = new std::vector<ozz::math::Float4x4>();
		model_pose->resize(skeleton->num_joints());

		GetAnimationManager().log->info("Allocated model pose with {} joints", skeleton->num_joints());
		return model_pose;
	}

	ozz::vector<Engine::Mesh>* AnimationManager::LoadMeshesFromPath(const std::string& path)
	{
		// Create a new vector to hold the meshes
		auto meshes = new ozz::vector<Engine::Mesh>();

		// Load the meshes from file
		if (!LoadMeshes(path.c_str(), meshes)) {
			GetAnimationManager().log->error("Failed to load meshes from path: {}", path);
			delete meshes;
			return nullptr;
		}

		GetAnimationManager().log->info("Loaded {} meshes from path: {}", meshes->size(), path);
		return meshes;
	}


} // namespace Engine


#include "assets/AssetManager.inl"