//
// Created by gabe on 6/24/25.
//

#include "components/Components.h"
#include "components/impl/AnimationComponent.h"


#include "ozz/animation/runtime/track.h"
#include "animation/AnimationManager.h"
#include "scripting/ScriptManager.h"

#include "animation/AnimationController.h"
#include "animation/AnimationPlayer.h"
#include "animation/Animation.h"
#include "ozz/animation/runtime/local_to_model_job.h"
#include "core/EngineData.h"
#include "core/Scene.h"

namespace Engine::Components {

    void AnimationComponent::OnAdded(Entity& entity)
    {
        if (!skeletonPath.empty()) {
            skeleton = GetAnimationManager().LoadSkeletonFromPath(skeletonPath);
            if (!skeleton) {
                spdlog::error("Failed to load skeleton from path: {}", skeletonPath);
            } else {
                GetDefaultLogger()->info("Loaded skeleton from path: {}", skeletonPath);
            }
        }

        if (skeleton != nullptr) {
            local_pose = AnimationManager::AllocateLocalPose(skeleton);
            model_pose = AnimationManager::AllocateModelPose(skeleton);

            if (!local_pose || !model_pose) {
                spdlog::error("Failed to allocate pose data for entity");
            }

            // Re-initialize any players that were deserialized before OnAdded ran
            for (auto& player : controller.players) {
                player->localPose.resize(skeleton->num_soa_joints());
                if (player->animation.IsValid()) {
                    Animation* anim = GetAssetManager().Get(player->animation);
                    if (anim && anim->source) {
                        player->context.Resize(anim->source->num_tracks());
                    }
                }
            }
        }
    }

    void AnimationComponent::SetSkeleton(const std::string& path)
    {
        this->skeletonPath = path;

        if (!skeletonPath.empty()) {
            skeleton = GetAnimationManager().LoadSkeletonFromPath(skeletonPath);
            if (!skeleton) {
                spdlog::error("Failed to load skeleton from path: {}", skeletonPath);
            } else {
                GetDefaultLogger()->info("Loaded skeleton from path: {}", skeletonPath);
            }
        }

        if (skeleton != nullptr) {
            delete local_pose;
            delete model_pose;

            local_pose = AnimationManager::AllocateLocalPose(skeleton);
            model_pose = AnimationManager::AllocateModelPose(skeleton);

            if (!local_pose || !model_pose) {
                spdlog::error("Failed to allocate pose data for entity");
            }

            // Re-initialize all existing players for the new skeleton
            for (auto& player : controller.players) {
                player->localPose.resize(skeleton->num_soa_joints());
                if (player->animation.IsValid()) {
                    Animation* anim = GetAssetManager().Get(player->animation);
                    if (anim && anim->source) {
                        player->context.Resize(anim->source->num_tracks());
                    }
                }
            }
        }
    }

    AnimationPlayer& AnimationComponent::PlayAnimation(const AssetHandle<Animation>& animation, bool loop)
    {
        auto& player = controller.players.emplace_back(std::make_unique<AnimationPlayer>());
        player->animation     = animation;
        player->looping       = loop;
        player->time          = 0.f;
        player->weight        = 1.f;
        player->targetWeight  = 1.f;
        player->playbackSpeed = 1.f;

        if (skeleton) {
            player->localPose.resize(skeleton->num_soa_joints());
        }

        if (animation.IsValid()) {
            Animation* anim = GetAssetManager().Get(animation);
            if (anim && anim->source) {
                player->context.Resize(anim->source->num_tracks());
                GetDefaultLogger()->info("Playing animation with {} tracks", anim->source->num_tracks());
            }
        }

        return *player;
    }

    void AnimationComponent::CrossfadeTo(const AssetHandle<Animation>& animation, float fadeDuration)
    {
        for (auto& player : controller.players) {
            player->targetWeight = 0.f;
        }

        AnimationPlayer& next = PlayAnimation(animation);
        next.weight       = 0.f;
        next.targetWeight = 1.f;

        controller.fadeDuration = fadeDuration;
    }

    AnimationPlayer* AnimationComponent::GetActivePlayer()
    {
        if (controller.players.empty()) return nullptr;

        AnimationPlayer* best = nullptr;
        for (auto& player : controller.players) {
            if (!best || player->weight > best->weight) {
                best = player.get();
            }
        }
        return best;
    }

    void AnimationComponent::OnRemoved(Entity& entity)
    {
        GetDefaultLogger()->info("DELETING ANIMATION POSES");

        if (local_pose) {
            delete local_pose;
            local_pose = nullptr;
        }

        if (model_pose) {
            delete model_pose;
            model_pose = nullptr;
        }

        controller.players.clear();
        skeleton = nullptr;
    }

    void AnimationComponent::CleanAnimationContexts()
    {
        Scene* scene = GetCurrentScene();
        if (!scene || !scene->GetRegistry()) {
            return;
        }

        auto view = scene->GetRegistry()->view<AnimationComponent>();
        for (auto entity : view) {
            auto& ac = view.get<AnimationComponent>(entity);

            ac.controller.players.clear();

            if (ac.local_pose) {
                delete ac.local_pose;
                ac.local_pose = nullptr;
            }
            if (ac.model_pose) {
                delete ac.model_pose;
                ac.model_pose = nullptr;
            }
            ac.skeleton = nullptr;
        }
    }

    static float animRatio = 0.0;

    void AnimationComponent::RenderInspector(Entity& entity)
    {
        ImGui::Text("Skeleton Information:");
        ImGui::Separator();
        ImGui::Text("Skeleton: %s", skeleton ? "Loaded" : "Null");
        ImGui::Text("Joints: %d", skeleton ? skeleton->num_joints() : 0);
        ImGui::Text("SOA Joints: %d", skeleton ? skeleton->num_soa_joints() : 0);

        ImGui::NewLine();
        ImGui::Text("Pose Information:");
        ImGui::Separator();
        ImGui::Text("Local Pose: %s", local_pose ? std::to_string(local_pose->size()).c_str() : "Null");
        ImGui::Text("Model Pose: %s", model_pose ? std::to_string(model_pose->size()).c_str() : "Null");

        ImGui::NewLine();
        ImGui::Text("Animation Controller:");
        ImGui::Separator();
        ImGui::Text("Active Players: %d", (int)controller.players.size());
        ImGui::Text("Fade Duration: %.2f", controller.fadeDuration);

        LeftLabelSliderFloat("Ratios", &animRatio, 0.f, 1.f);
        if(!controller.players.empty()) controller.players[0]->targetWeight = animRatio;
        if(controller.players.size() > 1) controller.players[1]->targetWeight = 1-animRatio;

        for (int i = 0; i < (int)controller.players.size(); ++i) {
            AnimationPlayer& player = *controller.players[i];
            ImGui::PushID(i);

            ImGui::Text("Player %d", i);
            ImGui::Indent();

            Animation* anim = player.animation.IsValid() ? GetAssetManager().Get(player.animation) : nullptr;
            ImGui::Text("Animation: %s", anim && anim->source ? "Loaded" : "Null");
            ImGui::Text("Tracks: %d",    anim && anim->source ? anim->source->num_tracks() : 0);
            ImGui::Text("Duration: %.2f", anim && anim->source ? anim->source->duration() : 0.f);
            ImGui::Text("Time: %.3f / Speed: %.2f", player.time, player.playbackSpeed);
            ImGui::Text("Weight: %.2f -> %.2f", player.weight, player.targetWeight);
            ImGui::Text("Looping: %s", player.looping ? "Yes" : "No");

            float ratio = (anim && anim->source && anim->source->duration() > 0.f)
                          ? player.time / anim->source->duration()
                          : 0.f;
            if (LeftLabelSliderFloat("Scrub", &ratio, 0.f, 1.f)) {
                if (anim && anim->source) {
                    player.time = ratio * anim->source->duration();
                }

                ozz::animation::SamplingJob sampling_job;
                sampling_job.animation = anim->source;
                sampling_job.context   = &player.context;
                sampling_job.ratio     = ratio;
                sampling_job.output    = ozz::make_span(player.localPose);
                if (!sampling_job.Run()) {
                    GetAnimationManager().log->error("Failed to sample animation (player {})", i);
                } else {
                    ozz::animation::LocalToModelJob ltm_job;
                    ltm_job.skeleton = skeleton;
                    ltm_job.input    = ozz::make_span(player.localPose);
                    ltm_job.output   = ozz::make_span(*model_pose);
                    if (!ltm_job.Run()) {
                        GetAnimationManager().log->error("Failed LocalToModel (player {})", i);
                    }
                }
            }

            AssetHandle<Animation> handle = player.animation;
            if (LeftLabelAssetAnimation("Animation", &handle)) {
                player.animation = handle;
                if (handle.IsValid()) {
                    Animation* newAnim = GetAssetManager().Get(handle);
                    if (newAnim && newAnim->source) {
                        player.context.Resize(newAnim->source->num_tracks());
                        if (skeleton) {
                            player.localPose.resize(skeleton->num_soa_joints());
                        }
                    }
                }
            }

            ImGui::Unindent();
            ImGui::PopID();
        }

        ImGui::NewLine();
        if (ImGui::Button("Add Player")) {
            PlayAnimation(AssetHandle<Animation>{});
        }
    }

} // namespace Engine::Components