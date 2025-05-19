#include "AnimationManager.h"

#include "components/Components.h"
#include "core/Engine.h"
#include "core/Entity.h"
#include "utils.h"

#include <spdlog/spdlog.h>

namespace Engine {

	AnimationManager::AnimationManager(GEngine* engine) : m_engine(engine), draw_skeleton_(false), draw_mesh_(true)
	{
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
	}

	bool AnimationManager::Initialize(Camera* camera)
	{
		camera_ = camera;

		if (!LoadAnimationAssets()) {
			spdlog::error("Failed to load animation assets");
			return false;
		}

		// Allocates and initializes renderer
		renderer_    = ozz::make_unique<RendererImpl>(camera_);
		bool success = renderer_->Initialize();
		if (!success) {
			spdlog::error("Failed to initialize animation renderer");
			return false;
		}
		else {
			spdlog::info("Initialized animated renderer");
		}

		return true;
	}

	bool AnimationManager::LoadAnimationAssets()
	{
		// spdlog::info("Loading animations");

		// // Reading skeleton
		// if (!LoadSkeleton("/home/gabe/CLionProjects/cpp-engine/resources/models/ruby_skeleton.ozz", &skeleton_)) {
		// 	return false;
		// }

		// // Reading animation
		// if (!LoadAnimation("/home/gabe/CLionProjects/cpp-engine/resources/models/ruby_animation.ozz", &animation_)) {
		// 	return false;
		// }

		// // Skeleton and animation needs to match
		// if (skeleton_.num_joints() != animation_.num_tracks()) {
		// 	spdlog::error("The provided animation doesn't match skeleton (joint count mismatch).");
		// 	return false;
		// }

		// // Allocates runtime buffers
		// const int num_soa_joints = skeleton_.num_soa_joints();
		// locals_.resize(num_soa_joints);
		// const int num_joints = skeleton_.num_joints();
		// models_.resize(num_joints);

		// // Allocates a context that matches animation requirements
		// context_.Resize(num_joints);

		// // Reading skinned meshes
		// if (!LoadMeshes("/home/gabe/CLionProjects/cpp-engine/resources/models/ruby_mesh.ozz", &meshes_)) {
		// 	return false;
		// }
		// else {
		// 	spdlog::info("Loaded meshes {}", meshes_.size());
		// }

		// // Computes the number of skinning matrices required to skin all meshes
		// // A mesh is skinned by only a subset of joints, so the number of skinning
		// // matrices might be less that the number of skeleton joints
		// // Mesh::joint_remaps is used to know how to order skinning matrices. So
		// // the number of matrices required is the size of joint_remaps
		// size_t num_skinning_matrices = 0;
		// for (const myns::Mesh& mesh : meshes_) {
		// 	num_skinning_matrices = ozz::math::Max(num_skinning_matrices, mesh.joint_remaps.size());
		// }

		// // Allocates skinning matrices
		// skinning_matrices_.resize(num_skinning_matrices);

		// // Check the skeleton matches with the mesh, especially that the mesh
		// // doesn't expect more joints than the skeleton has
		// for (const myns::Mesh& mesh : meshes_) {
		// 	if (num_joints < mesh.highest_joint_index()) {
		// 		spdlog::error("The provided mesh doesn't match skeleton (joint count mismatch).");
		// 		return false;
		// 	}
		// }

		return true;
	}

	void AnimationManager::Update(float deltaTime)
	{
		// Get all entities with a AnimationWorkerComponent and AnimationComponent and AnimationPoseComponent and SkeletonComponent
		auto view = m_engine->GetRegistry().view<Components::AnimationWorkerComponent, Components::AnimationComponent>();
		for (auto entity : view) {
			Entity e(entity, m_engine);
			auto&  animationWorkerComponent = e.GetComponent<Components::AnimationWorkerComponent>();
			auto&  animationComponent       = e.GetComponent<Components::AnimationComponent>();
			auto&  animationPoseComponent   = e.GetComponent<Components::AnimationPoseComponent>();
			auto&  skeletonComponent        = e.GetComponent<Components::SkeletonComponent>();

			// Update animation time
			controller_.set_time_ratio(sin(glfwGetTime() / 3.0f) / 2.0 + 0.5f);

			// Samples optimized animation at t = animation_time_
			ozz::animation::SamplingJob sampling_job;
			sampling_job.animation = animationComponent.animation;
			sampling_job.context   = animationWorkerComponent.context;
			sampling_job.ratio     = controller_.time_ratio();
			sampling_job.output    = ozz::make_span(*animationPoseComponent.local_pose);
			if (!sampling_job.Run()) {
				spdlog::error("Failed to sample animation");
				return;
			}

			ozz::animation::LocalToModelJob ltm_job;
			ltm_job.skeleton = skeletonComponent.skeleton;
			ltm_job.input    = ozz::make_span(*animationPoseComponent.local_pose);
			ltm_job.output   = ozz::make_span(*animationPoseComponent.model_pose);
			if (!ltm_job.Run()) {
				spdlog::error("Failed to convert to model space");
				return;
			}
		}


		// // Update animation time
		// controller_.set_time_ratio(sin(glfwGetTime() / 3.0f) / 2.0 + 0.5f);

		// // Samples optimized animation at t = animation_time_
		// ozz::animation::SamplingJob sampling_job;
		// sampling_job.animation = &animation_;
		// sampling_job.context   = &context_;
		// sampling_job.ratio     = controller_.time_ratio();
		// sampling_job.output    = make_span(locals_);
		// if (!sampling_job.Run()) {
		// 	spdlog::error("Failed to sample animation");
		// 	return;
		// }

		// // Converts from local space to model space matrices
		// ozz::animation::LocalToModelJob ltm_job;
		// ltm_job.skeleton = &skeleton_;
		// ltm_job.input    = make_span(locals_);
		// ltm_job.output   = make_span(models_);
		// if (!ltm_job.Run()) {
		// 	spdlog::error("Failed to convert to model space");
		// 	return;
		// }
	}


	ozz::math::Float4x4 FromMatrix(const glm::mat4& glmMatrix)
	{
		return ozz::math::Float4x4{ozz::math::simd_float4::Load(glmMatrix[0][0], glmMatrix[0][1], glmMatrix[0][2], glmMatrix[0][3]),
		                           ozz::math::simd_float4::Load(glmMatrix[1][0], glmMatrix[1][1], glmMatrix[1][2], glmMatrix[1][3]),
		                           ozz::math::simd_float4::Load(glmMatrix[2][0], glmMatrix[2][1], glmMatrix[2][2], glmMatrix[2][3]),
		                           ozz::math::simd_float4::Load(glmMatrix[3][0], glmMatrix[3][1], glmMatrix[3][2], glmMatrix[3][3])};
	}


	void AnimationManager::Render()
	{
		// Get all entities with a SkinnedMeshComponent and transform and AnimationPoseComponent
		auto view = m_engine->GetRegistry().view<Components::SkinnedMeshComponent, Components::Transform>();
		for (auto entity : view) {
			bool                      success = true;
			Entity                    e(entity, m_engine);
			auto&                     skinnedMeshComponent   = e.GetComponent<Components::SkinnedMeshComponent>();
			auto&                     animationPoseComponent = e.GetComponent<Components::AnimationPoseComponent>();
			const ozz::math::Float4x4 transform              = FromMatrix(e.GetComponent<Components::Transform>().GetMatrix());

			// Render each mesh
			for (const myns::Mesh& mesh : *skinnedMeshComponent.meshes) {
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

		// if (draw_mesh_) {
		// 	// Builds skinning matrices, based on the output of the animation stage
		// 	// The mesh might not use (aka be skinned by) all skeleton joints. We
		// 	// use the joint remapping table (available from the mesh object) to
		// 	// reorder model-space matrices and build skinning ones
		// 	for (const myns::Mesh& mesh : meshes_) {
		// 		for (size_t i = 0; i < mesh.joint_remaps.size(); ++i) {
		// 			skinning_matrices_[i] = models_[mesh.joint_remaps[i]] * mesh.inverse_bind_poses[i];
		// 		}

		// 		// Renders skin
		// 		success &= renderer_->DrawSkinnedMesh(mesh, make_span(skinning_matrices_), transform, render_options_);
		// 	}
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
			spdlog::error("Failed to load skeleton from path: {}", path);
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
			spdlog::error("Failed to load animation from path: {}", path);
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
			spdlog::error("Cannot allocate local pose for null skeleton");
			return nullptr;
		}

		// Create a new vector of SoaTransforms with the correct size
		auto local_pose = new std::vector<ozz::math::SoaTransform>();
		local_pose->resize(skeleton->num_soa_joints());

		spdlog::info("Allocated local pose with {} SoA joints", skeleton->num_soa_joints());
		return local_pose;
	}

	std::vector<ozz::math::Float4x4>* AnimationManager::AllocateModelPose(const ozz::animation::Skeleton* skeleton)
	{
		if (!skeleton) {
			spdlog::error("Cannot allocate model pose for null skeleton");
			return nullptr;
		}

		// Create a new vector of Float4x4 matrices with the correct size
		auto model_pose = new std::vector<ozz::math::Float4x4>();
		model_pose->resize(skeleton->num_joints());

		spdlog::info("Allocated model pose with {} joints", skeleton->num_joints());
		return model_pose;
	}

	ozz::vector<myns::Mesh>* AnimationManager::LoadMeshesFromPath(const std::string& path)
	{
		// Create a new vector to hold the meshes
		auto meshes = new ozz::vector<myns::Mesh>();

		// Load the meshes from file
		if (!LoadMeshes(path.c_str(), meshes)) {
			spdlog::error("Failed to load meshes from path: {}", path);
			delete meshes;
			return nullptr;
		}

		spdlog::info("Loaded {} meshes from path: {}", meshes->size(), path);
		return meshes;
	}

} // namespace Engine
