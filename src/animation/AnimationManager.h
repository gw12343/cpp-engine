#pragma once

#include <memory>
#include <vector>
#include <string>

#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/animation/runtime/track.h>
#include <ozz/animation/runtime/track_sampling_job.h>
#include <ozz/animation/runtime/track_triggering_job.h>
#include <ozz/animation/runtime/blending_job.h>
#include <ozz/base/containers/vector.h>
#include <ozz/base/memory/unique_ptr.h>
#include <ozz/base/maths/soa_transform.h>
#include <ozz/base/maths/vec_float.h>
#include <ozz/base/maths/simd_math.h>

#include "animation/renderer.h"
#include "animation/renderer_impl.h"
#include "animation/mesh.h"
#include "Camera.h"

namespace Engine {

class AnimationManager {
public:
    AnimationManager();
    ~AnimationManager() = default;

    bool Initialize(Camera* camera);
    void Update(float deltaTime);
    void Render();

    // Getters for animation properties
    ozz::animation::Skeleton& GetSkeleton() { return skeleton_; }
    const ozz::vector<myns::Mesh>& GetMeshes() const { return meshes_; }
    bool& GetDrawSkeleton() { return draw_skeleton_; }
    bool& GetDrawMesh() { return draw_mesh_; }
    AnimatedRenderer::Options& GetRenderOptions() { return render_options_; }

private:
    bool LoadAnimationAssets();
    
    // Animation properties
    ozz::animation::Skeleton skeleton_;
    ozz::animation::Animation animation_;
    ozz::animation::SamplingJob::Context context_;
    ozz::vector<ozz::math::SoaTransform> locals_;
    ozz::vector<ozz::math::Float4x4> models_;
    ozz::vector<myns::Mesh> meshes_;
    ozz::vector<ozz::math::Float4x4> skinning_matrices_;
    
    // Animation controller
    class PlaybackController {
    public:
        PlaybackController() : time_ratio_(0.f) {}
        
        float time_ratio() const { return time_ratio_; }
        void set_time_ratio(float _ratio) { time_ratio_ = _ratio; }
        
    private:
        float time_ratio_;
    };
    
    PlaybackController controller_;
    
    // Rendering options
    bool draw_skeleton_ = false;
    bool draw_mesh_ = true;
    AnimatedRenderer::Options render_options_;
    
    // Renderer
    ozz::unique_ptr<RendererImpl> renderer_;
    Camera* camera_ = nullptr;
};

}