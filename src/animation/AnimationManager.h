#pragma once

#include "Camera.h"
#include "animation/AnimatedMesh.h"
#include "animation/renderer.h"
#include "animation/renderer_impl.h"
#include "core/module/Module.h"


#include <ozz/animation/runtime/animation.h>

#include <ozz/animation/runtime/skeleton.h>

#include "ozz/animation/runtime/track.h"
#include "ozz/base/containers/vector.h"
#include "ozz/base/maths/simd_math.h"

#include <ozz/base/maths/soa_transform.h>

#include <ozz/base/memory/unique_ptr.h>

#include <unordered_map>


namespace Engine {

	class GEngine; // Forward declaration

	class AnimationManager : public Module {
	  public:
		void        onInit() override;
		void        onUpdate(float dt) override;
		void        onGameStart() override {}
		void        onShutdown() override;
		std::string name() const override { return "AnimationModule"; }

		static std::vector<Mesh, ozz::StdAllocator<Mesh>>* LoadMeshesFromPath(const std::string& string) { return nullptr; }



		void Render();


		bool&                      GetDrawSkeleton() { return draw_skeleton_; }
		bool&                      GetDrawMesh() { return draw_mesh_; }
		AnimatedRenderer::Options& GetRenderOptions() { return render_options_; }

		// Load a skeleton from a file path
		ozz::animation::Skeleton* LoadSkeletonFromPath(const std::string& path);

		// Load an animation from a file path
		ozz::animation::Animation* LoadAnimationFromPath(const std::string& path);

		// Allocate local pose data for a skeleton
		static std::vector<ozz::math::SoaTransform>* AllocateLocalPose(const ozz::animation::Skeleton* skeleton);

		// Allocate model pose data for a skeleton
		static std::vector<ozz::math::Float4x4>* AllocateModelPose(const ozz::animation::Skeleton* skeleton);

		// Load meshes from a file path
		//static ozz::vector<Engine::Mesh>* LoadMeshesFromPath(const std::string& path);


		ozz::unique_ptr<RendererImpl> renderer_;

	  private:
		// Map to store loaded skeletons
		std::unordered_map<std::string, std::unique_ptr<ozz::animation::Skeleton>> loaded_skeletons_;

		// Map to store loaded animations
		// std::unordered_map<std::string, std::unique_ptr<ozz::animation::Animation>> loaded_animations_;


		// Rendering options
		bool                      draw_skeleton_ = false;
		bool                      draw_mesh_     = true;
		AnimatedRenderer::Options render_options_;

		// Renderer
	};

} // namespace Engine
