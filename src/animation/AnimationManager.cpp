#include "AnimationManager.h"

#include "AnimationUtils.h"
#include "Animation.h"
#include "core/Entity.h"
#include "core/EngineData.h"

#include "components/impl/AnimationComponent.h"
#include "components/impl/TransformComponent.h"
#include "components/impl/SkinnedMeshComponent.h"

#include <utils/Utils.h>
#include <components/impl/EntityMetadataComponent.h>

#include "core/SceneManager.h"
#include "physics/PhysicsManager.h"
#include "entt/entt.hpp"
#include "ozz/animation/runtime/local_to_model_job.h"
#include "assets/AssetManager.h"
#include "scripting/ScriptManager.h"
#include <algorithm>
#include <cmath>

namespace Engine {

    void AnimationManager::setLuaBindings()
    {
        auto& lua = GetScriptManager().lua;

        lua.new_usertype<AnimationManager>(
            "AnimationManager",
            "getDrawSkeleton", &AnimationManager::GetDrawSkeletonValue,
            "setDrawSkeleton", &AnimationManager::SetDrawSkeleton,
            "getDrawMesh", &AnimationManager::GetDrawMeshValue,
            "setDrawMesh", &AnimationManager::SetDrawMesh,
            "drawSkeleton",
            sol::property(&AnimationManager::GetDrawSkeletonValue, &AnimationManager::SetDrawSkeleton),
            "drawMesh",
            sol::property(&AnimationManager::GetDrawMeshValue, &AnimationManager::SetDrawMesh));

        lua.set_function("getAnimationManager", []() -> AnimationManager& {
            return Engine::GetAnimationManager();
        });

        lua.set_function("loadAnimation", [](const std::string& path) -> AnimationHandle {
            return GetAssetManager().Load<Animation>(path);
        });
    }

    void AnimationManager::onInit()
    {
        draw_skeleton_ = false;
        draw_mesh_     = true;

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

        if (!renderer_->Initialize()) {
            log->error("Failed to initialize animation renderer");
        } else {
            log->info("Initialized animated renderer");
        }
    }

    void AnimationManager::onShutdown()
    {
        if (Get().assetManager) {
            GetAssetManager().UnloadAll<Animation>();
        }

        renderer_.reset();
        loaded_skeletons_.clear();
    }

    void AnimationManager::onUpdate(float deltaTime)
    {
        ZoneScopedN("Animation Update");

        auto animationView =
            GetCurrentSceneRegistry().view<Components::EntityMetadata, Components::AnimationComponent>();

        for (auto [entity, metadata, ac] : animationView.each()) {
            if (!ac.skeleton) continue;

            // Advance time only while playing; always evaluate pose for editor display.
            const float dt = (GetState() == PLAYING) ? deltaTime : 0.f;
            if (ac.IsPlaying()) {
                ac.UpdatePlayback(dt);
            } else if (ac.local_pose && ac.model_pose) {
                ac.EvaluatePose();
            }
        }
    }

    void AnimationManager::Render()
    {
        ZoneScopedN("AnimationManager::Render (GBuffer)");
        auto view = GetCurrentSceneRegistry().view<Components::SkinnedMeshComponent,
                Components::AnimationComponent,
                Components::Transform>();
        for (auto entity : view) {
            ZoneScopedN("GBuffer Skinned Entity");
            Entity e(entity, GetCurrentScene());
            auto&  skinnedMeshComponent = e.GetComponent<Components::SkinnedMeshComponent>();
            auto&  animationComponent   = e.GetComponent<Components::AnimationComponent>();
            const ozz::math::Float4x4 transform = FromMatrix(e.GetComponent<Components::Transform>().GetWorldMatrix());

            if (!skinnedMeshComponent.visible) continue;
            if (!animationComponent.model_pose || !skinnedMeshComponent.meshes || !skinnedMeshComponent.skinning_matrices)
                continue;

            for (const Engine::AnimatedMesh& mesh : *skinnedMeshComponent.meshes) {
                {
                    ZoneScopedN("GBuffer Build Skinning Matrices");
                    for (size_t i = 0; i < mesh.joint_remaps.size(); ++i) {
                        (*skinnedMeshComponent.skinning_matrices)[i] =
                                (*animationComponent.model_pose)[mesh.joint_remaps[i]] * mesh.inverse_bind_poses[i];
                    }
                }
                renderer_->DrawSkinnedMesh(mesh, ozz::make_span(*skinnedMeshComponent.skinning_matrices),
                                           transform, skinnedMeshComponent.meshMaterial, render_options_);
            }
        }
    }

    void AnimationManager::RenderDebug() const
    {
        // auto view = GetCurrentSceneRegistry().view<Components::AnimationComponent, Components::Transform>();
        // for (auto entity : view) {
        //     Entity e(entity, GetCurrentScene());
        //     auto&  animationComponent = e.GetComponent<Components::AnimationComponent>();
        //     const ozz::math::Float4x4 transform = FromMatrix(e.GetComponent<Components::Transform>().GetWorldMatrix());
        //
        //     if (!animationComponent.skeleton || !animationComponent.model_pose) continue;
        //     renderer_->DrawPosture(*animationComponent.skeleton,
        //                            ozz::make_span(*animationComponent.model_pose),
        //                            transform, true);
        // }
    }

    ozz::animation::Skeleton* AnimationManager::LoadSkeletonFromPath(const std::string& path)
    {
        auto it = loaded_skeletons_.find(path);
        if (it != loaded_skeletons_.end()) {
            return it->second.get();
        }

        auto skeleton = std::make_unique<ozz::animation::Skeleton>();
        if (!LoadSkeleton(path.c_str(), skeleton.get())) {
            log->error("Failed to load skeleton from path: {}", path);
            return nullptr;
        }

        ozz::animation::Skeleton* result = skeleton.get();
        loaded_skeletons_[path]          = std::move(skeleton);
        return result;
    }

    ozz::animation::Animation* AnimationManager::LoadAnimationFromPath(const std::string& path)
    {
        auto animation = new ozz::animation::Animation();
        if (!LoadAnimation(path.c_str(), animation)) {
            log->error("Failed to load animation from path: {}", path);
            delete animation;
            return nullptr;
        }
        return animation;
    }

    std::vector<ozz::math::SoaTransform>* AnimationManager::AllocateLocalPose(const ozz::animation::Skeleton* skeleton)
    {
        if (!skeleton) {
            GetAnimationManager().log->error("Cannot allocate local pose for null skeleton");
            return nullptr;
        }
        auto local_pose = new std::vector<ozz::math::SoaTransform>();
        local_pose->resize(skeleton->num_soa_joints());
        return local_pose;
    }

    std::vector<ozz::math::Float4x4>* AnimationManager::AllocateModelPose(const ozz::animation::Skeleton* skeleton)
    {
        if (!skeleton) {
            GetAnimationManager().log->error("Cannot allocate model pose for null skeleton");
            return nullptr;
        }
        auto model_pose = new std::vector<ozz::math::Float4x4>();
        model_pose->resize(skeleton->num_joints());
        return model_pose;
    }

    ozz::vector<AnimatedMesh>* AnimationManager::LoadMeshesFromPath(std::string path)
    {
        auto meshes = new ozz::vector<Engine::AnimatedMesh>();
        if (!LoadMeshes(path.c_str(), meshes)) {
            GetAnimationManager().log->error("Failed to load meshes from path: {}", path);
            delete meshes;
            return nullptr;
        }
        return meshes;
    }

} // namespace Engine

#include "assets/AssetManager.inl"