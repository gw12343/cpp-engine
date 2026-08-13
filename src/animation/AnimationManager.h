#pragma once

#include "Camera.h"
#include "animation/rendering/AnimatedMesh.h"
#include "animation/rendering/renderer_impl.h"
#include "core/module/Module.h"


#include <ozz/animation/runtime/animation.h>

#include <ozz/animation/runtime/skeleton.h>

#include "ozz/base/containers/vector.h"
#include "ozz/base/maths/simd_math.h"

#include <ozz/base/maths/soa_transform.h>

#include <ozz/base/memory/unique_ptr.h>

#include <unordered_map>


namespace Engine {


	class AnimationManager : public Module {
	  public:
		void        onInit() override;
		void        onUpdate(float dt) override;
		void        onGameStart() override {}
		void        onShutdown() override;
		void        setLuaBindings() override;
		std::string name() const override { return "AnimationModule"; }

		void Render();
		void RenderDebug() const;

		/// Once per frame: build skinning matrices + CPU-skin every skinned mesh (parallelized).
		/// Call before shadow / GBuffer / mouse-pick draws that need skinned geometry.
		void PrepareSkinnedMeshes();
		[[nodiscard]] uint64_t GetPoseGeneration() const { return pose_generation_; }

		bool&                  GetDrawSkeleton() { return draw_skeleton_; }
		bool&                  GetDrawMesh() { return draw_mesh_; }
		bool                   GetDrawSkeletonValue() const { return draw_skeleton_; }
		bool                   GetDrawMeshValue() const { return draw_mesh_; }
		void                   SetDrawSkeleton(bool v) { draw_skeleton_ = v; }
		void                   SetDrawMesh(bool v) { draw_mesh_ = v; }
		RendererImpl::Options& GetRenderOptions() { return render_options_; }

		// Load a skeleton from a file path
		ozz::animation::Skeleton* LoadSkeletonFromPath(const std::string& path);

		// Load an animation from a file path
		ozz::animation::Animation* LoadAnimationFromPath(const std::string& path);

		// Allocate local pose data for a skeleton
		static std::vector<ozz::math::SoaTransform>* AllocateLocalPose(const ozz::animation::Skeleton* skeleton);

		// Allocate model pose data for a skeleton
		static std::vector<ozz::math::Float4x4>* AllocateModelPose(const ozz::animation::Skeleton* skeleton);

		// Load meshes from a file path
		static ozz::vector<AnimatedMesh>* LoadMeshesFromPath(std::string path);


		ozz::unique_ptr<RendererImpl> renderer_;

	  private:
		// Map to store loaded skeletons (legacy; primary cache is AssetManager)
		std::unordered_map<std::string, std::unique_ptr<ozz::animation::Skeleton>> loaded_skeletons_;

		// Rendering options
		bool                      draw_skeleton_ = false;
		bool                      draw_mesh_     = true;
		RendererImpl::Options render_options_;

		/// Bumped each animation update; PrepareSkinnedMeshes rebuilds when this changes.
		uint64_t pose_generation_    = 0;
		uint64_t skinned_generation_ = ~uint64_t{0};
	};

} // namespace Engine
