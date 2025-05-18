#include "AnimationManager.h"

#include "utils.h"

#include <spdlog/spdlog.h>

namespace Engine {

	AnimationManager::AnimationManager() : draw_skeleton_(false), draw_mesh_(true)
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
		spdlog::info("Loading animations");

		// Reading skeleton
		if (!LoadSkeleton("/home/gabe/CLionProjects/cpp-engine/resources/models/ruby_skeleton.ozz", &skeleton_)) {
			return false;
		}

		// Reading animation
		if (!LoadAnimation("/home/gabe/CLionProjects/cpp-engine/resources/models/ruby_animation.ozz", &animation_)) {
			return false;
		}

		// Skeleton and animation needs to match
		if (skeleton_.num_joints() != animation_.num_tracks()) {
			spdlog::error("The provided animation doesn't match skeleton (joint count mismatch).");
			return false;
		}

		// Allocates runtime buffers
		const int num_soa_joints = skeleton_.num_soa_joints();
		locals_.resize(num_soa_joints);
		const int num_joints = skeleton_.num_joints();
		models_.resize(num_joints);

		// Allocates a context that matches animation requirements
		context_.Resize(num_joints);

		// Reading skinned meshes
		if (!LoadMeshes("/home/gabe/CLionProjects/cpp-engine/resources/models/ruby_mesh.ozz", &meshes_)) {
			return false;
		}
		else {
			spdlog::info("Loaded meshes {}", meshes_.size());
		}

		// Computes the number of skinning matrices required to skin all meshes
		// A mesh is skinned by only a subset of joints, so the number of skinning
		// matrices might be less that the number of skeleton joints
		// Mesh::joint_remaps is used to know how to order skinning matrices. So
		// the number of matrices required is the size of joint_remaps
		size_t num_skinning_matrices = 0;
		for (const myns::Mesh& mesh : meshes_) {
			num_skinning_matrices = ozz::math::Max(num_skinning_matrices, mesh.joint_remaps.size());
		}

		// Allocates skinning matrices
		skinning_matrices_.resize(num_skinning_matrices);

		// Check the skeleton matches with the mesh, especially that the mesh
		// doesn't expect more joints than the skeleton has
		for (const myns::Mesh& mesh : meshes_) {
			if (num_joints < mesh.highest_joint_index()) {
				spdlog::error("The provided mesh doesn't match skeleton (joint count mismatch).");
				return false;
			}
		}

		return true;
	}

	void AnimationManager::Update(float deltaTime)
	{
		// Update animation time
		controller_.set_time_ratio(sin(glfwGetTime() / 3.0f) / 2.0 + 0.5f);

		// Samples optimized animation at t = animation_time_
		ozz::animation::SamplingJob sampling_job;
		sampling_job.animation = &animation_;
		sampling_job.context   = &context_;
		sampling_job.ratio     = controller_.time_ratio();
		sampling_job.output    = make_span(locals_);
		if (!sampling_job.Run()) {
			spdlog::error("Failed to sample animation");
			return;
		}

		// Converts from local space to model space matrices
		ozz::animation::LocalToModelJob ltm_job;
		ltm_job.skeleton = &skeleton_;
		ltm_job.input    = make_span(locals_);
		ltm_job.output   = make_span(models_);
		if (!ltm_job.Run()) {
			spdlog::error("Failed to convert to model space");
			return;
		}
	}

	void AnimationManager::Render()
	{
		bool                      success   = true;
		const ozz::math::Float4x4 transform = ozz::math::Float4x4::identity();

		if (draw_skeleton_) {
			success &= renderer_->DrawPosture(skeleton_, make_span(models_), transform, true);
		}

		if (draw_mesh_) {
			// Builds skinning matrices, based on the output of the animation stage
			// The mesh might not use (aka be skinned by) all skeleton joints. We
			// use the joint remapping table (available from the mesh object) to
			// reorder model-space matrices and build skinning ones
			for (const myns::Mesh& mesh : meshes_) {
				for (size_t i = 0; i < mesh.joint_remaps.size(); ++i) {
					skinning_matrices_[i] = models_[mesh.joint_remaps[i]] * mesh.inverse_bind_poses[i];
				}

				// Renders skin
				success &= renderer_->DrawSkinnedMesh(mesh, make_span(skinning_matrices_), transform, render_options_);
			}
		}
	}

} // namespace Engine