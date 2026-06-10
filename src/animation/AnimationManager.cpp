#include "AnimationManager.h"

#include "AnimationUtils.h"
#include "core/Entity.h"

#include "components/impl/AnimationComponent.h"
#include "components/impl/TransformComponent.h"
#include "components/impl/SkinnedMeshComponent.h"

#include <utils/Utils.h>
#include <components/impl/EntityMetadataComponent.h>

#include "core/SceneManager.h"
#include "physics/PhysicsManager.h"
#include "entt/entt.hpp"
#include "ozz/animation/runtime/local_to_model_job.h"

namespace Engine {

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
        for (auto& pair : loaded_skeletons_) {
            pair.second.reset();
        }
    }

    void AnimationManager::onUpdate(float deltaTime)
    {
        ZoneScopedN("Animation Update");



        {
            ZoneScopedN("Sample animations");
            auto animationView = GetCurrentSceneRegistry().view<Components::EntityMetadata, Components::AnimationComponent>();
            for (auto [entity, metadata, ac] : animationView.each()) {
                if (!ac.skeleton || !ac.local_pose || !ac.model_pose) continue;

                const int soa_count = ac.skeleton->num_soa_joints();

                // ----------------------------------------------------------------
                // 1. Advance time and sample each player
                // ----------------------------------------------------------------
                std::vector<ozz::animation::BlendingJob::Layer> layers;

                for (auto& player : ac.controller.players) {
                    Animation* anim = player->animation.IsValid()
                                      ? GetAssetManager().Get(player->animation)
                                      : nullptr;

                    if (!anim || !anim->source) continue;

                    // Advance time
                    if (GetState() == PLAYING) {
                        player->time += deltaTime * player->playbackSpeed;
                        if (player->looping) {
                            player->time = fmod(player->time, anim->source->duration());
                        } else {
                            player->time = std::min(player->time, anim->source->duration());
                        }
                    }

                    // Lerp weight toward target (crossfade)
                    constexpr float kWeightLerpSpeed = 5.f;
                    player->weight = std::lerp(player->weight, player->targetWeight,
                                               std::min(1.f, deltaTime * kWeightLerpSpeed));

                    // Ensure localPose is sized correctly
                    if ((int)player->localPose.size() != soa_count) {
                        player->localPose.resize(soa_count);
                    }

                    // Sample
                    const float ratio = anim->source->duration() > 0.f
                                        ? player->time / anim->source->duration()
                                        : 0.f;

                    ozz::animation::SamplingJob sampling_job;
                    sampling_job.animation = anim->source;
                    sampling_job.context   = &player->context;
                    sampling_job.ratio     = ratio;
                    sampling_job.output    = ozz::make_span(player->localPose);

                    if (!sampling_job.Run()) {
                        log->error("Failed to sample animation player");
                        continue;
                    }

                    // Register layer for blending
                    ozz::animation::BlendingJob::Layer layer;
                    layer.transform = ozz::make_span(player->localPose);
                    layer.weight    = player->weight;
                    layers.push_back(layer);
                }

                // Blend all layers into local_pose
                if (layers.empty()) continue;

                if (layers.size() == 1 && layers[0].weight >= 1.f) {
                    // skip blending job
                    ozz::span<const ozz::math::SoaTransform> src = layers[0].transform;
                    std::copy(src.begin(), src.end(), ac.local_pose->begin());
                } else {
                    ozz::animation::BlendingJob blend_job;
                    blend_job.threshold = ozz::animation::BlendingJob().threshold; // default threshold
                    blend_job.layers    = ozz::make_span(layers);
                    blend_job.rest_pose = ac.skeleton->joint_rest_poses();
                    blend_job.output    = ozz::make_span(*ac.local_pose);

                    if (!blend_job.Run()) {
                        log->error("Failed to blend animation layers");
                        continue;
                    }
                }

                // Local -> model space
                ozz::animation::LocalToModelJob ltm_job;
                ltm_job.skeleton = ac.skeleton;
                ltm_job.input    = ozz::make_span(*ac.local_pose);
                ltm_job.output   = ozz::make_span(*ac.model_pose);

                if (!ltm_job.Run()) {
                    log->error("Failed to convert to model space");
                    continue;
                }

                // Prune players

//                auto& players = ac.controller.players;
//                players.erase(
//                        std::remove_if(players.begin(), players.end(),
//                                       [](const std::unique_ptr<AnimationPlayer>& p) {
//                                           return p->weight < 0.001f && p->targetWeight <= 0.f;
//                                       }),
//                        players.end());
            }
        }
    }

    void AnimationManager::Render()
    {
        auto view = GetCurrentSceneRegistry().view<Components::SkinnedMeshComponent,
                Components::AnimationComponent,
                Components::Transform>();
        for (auto entity : view) {
            Entity e(entity, GetCurrentScene());
            auto&  skinnedMeshComponent = e.GetComponent<Components::SkinnedMeshComponent>();
            auto&  animationComponent   = e.GetComponent<Components::AnimationComponent>();
            const ozz::math::Float4x4 transform = FromMatrix(e.GetComponent<Components::Transform>().GetWorldMatrix());

            if (!skinnedMeshComponent.visible) continue;

            for (const Engine::AnimatedMesh& mesh : *skinnedMeshComponent.meshes) {
                for (size_t i = 0; i < mesh.joint_remaps.size(); ++i) {
                    (*skinnedMeshComponent.skinning_matrices)[i] =
                            (*animationComponent.model_pose)[mesh.joint_remaps[i]] * mesh.inverse_bind_poses[i];
                }
                renderer_->DrawSkinnedMesh(mesh, ozz::make_span(*skinnedMeshComponent.skinning_matrices),
                                           transform, skinnedMeshComponent.meshMaterial, render_options_);
            }
        }
    }

    void AnimationManager::RenderDebug() const
    {
        auto view = GetCurrentSceneRegistry().view<Components::AnimationComponent, Components::Transform>();
        for (auto entity : view) {
            Entity e(entity, GetCurrentScene());
            auto&  animationComponent = e.GetComponent<Components::AnimationComponent>();
            const ozz::math::Float4x4 transform = FromMatrix(e.GetComponent<Components::Transform>().GetWorldMatrix());

            renderer_->DrawPosture(*animationComponent.skeleton,
                                   ozz::make_span(*animationComponent.model_pose),
                                   transform, true);
        }
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