//
// AnimationComponent — dual-slot full-body playback on top of ozz.
//

#include "components/impl/AnimationComponent.h"

#include "animation/AnimationManager.h"
#include "animation/Animation.h"
#include "scripting/ScriptManager.h"
#include "core/EngineData.h"
#include "core/Scene.h"
#include "assets/AssetManager.h"

#include "ozz/animation/runtime/local_to_model_job.h"
#include "ozz/animation/runtime/blending_job.h"
#include "ozz/animation/runtime/sampling_job.h"
#include "ozz/animation/runtime/skeleton.h"

#include <algorithm>
#include <cmath>

namespace Engine::Components {

	AnimationComponent::AnimationComponent(const AnimationComponent& other)
	{
		skeletonPath          = other.skeletonPath;
		defaultFadeDuration   = other.defaultFadeDuration;
		playbackSpeed         = other.playbackSpeed;
		counteractRootOffset  = other.counteractRootOffset;
		current.clip          = other.current.clip;
		current.time          = other.current.time;
		current.speed         = other.current.speed;
		current.looping       = other.current.looping;
		current.active        = other.current.active;
		// Runtime buffers rebuilt in OnAdded
	}

	AnimationComponent& AnimationComponent::operator=(const AnimationComponent& other)
	{
		if (this != &other) {
			skeletonPath         = other.skeletonPath;
			defaultFadeDuration  = other.defaultFadeDuration;
			playbackSpeed        = other.playbackSpeed;
			counteractRootOffset = other.counteractRootOffset;
			current.clip         = other.current.clip;
			current.time         = other.current.time;
			current.speed        = other.current.speed;
			current.looping      = other.current.looping;
			current.active       = other.current.active;
			from.Deactivate();
			isFading       = false;
			blendWeight    = 1.f;
			fadeElapsed    = 0.f;
			restRootValid  = false;
		}
		return *this;
	}

	void AnimationComponent::EnsurePoseBuffers()
	{
		if (!skeleton) return;
		if (!local_pose) {
			local_pose = AnimationManager::AllocateLocalPose(skeleton);
		}
		if (!model_pose) {
			model_pose = AnimationManager::AllocateModelPose(skeleton);
		}
	}

	Animation* AnimationComponent::ResolveClip(const AnimationTrack& track)
	{
		if (!track.clip.IsValid()) {
			return nullptr;
		}
		Animation* clip = GetAssetManager().Get(track.clip);
		return (clip && clip->IsValid()) ? clip : nullptr;
	}

	bool AnimationComponent::PrepareTrack(AnimationTrack& track)
	{
		if (!skeleton) {
			return false;
		}
		Animation* clip = ResolveClip(track);
		if (!clip) {
			return false;
		}

		const int soa = skeleton->num_soa_joints();
		if ((int)track.localPose.size() != soa) {
			track.localPose.resize(soa);
		}
		track.context.Resize(clip->NumTracks());
		track.active = true;
		return true;
	}

	void AnimationComponent::OnAdded(Entity& entity)
	{
		if (!skeletonPath.empty()) {
			skeleton = GetAnimationManager().LoadSkeletonFromPath(skeletonPath);
			if (!skeleton) {
				spdlog::error("Failed to load skeleton from path: {}", skeletonPath);
			}
		}

		if (skeleton) {
			EnsurePoseBuffers();
			CacheRestRootModelTranslation();
			if (current.clip.IsValid()) {
				if (PrepareTrack(current)) {
					// Keep serialized time; clamp to clip length after prepare
					if (Animation* clip = ResolveClip(current)) {
						const float dur = clip->Duration();
						if (dur > 0.f) {
							current.time = std::fmod(current.time, dur);
							if (current.time < 0.f) current.time += dur;
						}
					}
				} else {
					current.Deactivate();
				}
			}
			from.Deactivate();
			isFading    = false;
			blendWeight = 1.f;
			EvaluatePose();
		}
	}

	void AnimationComponent::OnRemoved(Entity& entity)
	{
		delete local_pose;
		local_pose = nullptr;
		delete model_pose;
		model_pose = nullptr;
		current.Deactivate();
		from.Deactivate();
		skeleton = nullptr;
	}

	void AnimationComponent::CleanAnimationContexts()
	{
		Scene* scene = GetCurrentScene();
		if (!scene || !scene->GetRegistry()) return;

		auto view = scene->GetRegistry()->view<AnimationComponent>();
		for (auto entity : view) {
			auto& ac = view.get<AnimationComponent>(entity);
			delete ac.local_pose;
			ac.local_pose = nullptr;
			delete ac.model_pose;
			ac.model_pose = nullptr;
			ac.current.Deactivate();
			ac.from.Deactivate();
			ac.skeleton = nullptr;
			ac.isFading = false;
		}
	}

	void AnimationComponent::SetSkeleton(const std::string& path)
	{
		skeletonPath  = path;
		skeleton      = nullptr;
		restRootValid = false;
		if (!skeletonPath.empty()) {
			skeleton = GetAnimationManager().LoadSkeletonFromPath(skeletonPath);
		}

		delete local_pose;
		local_pose = nullptr;
		delete model_pose;
		model_pose = nullptr;

		if (skeleton) {
			EnsurePoseBuffers();
			CacheRestRootModelTranslation();
			if (current.active) {
				PrepareTrack(current);
			}
			if (from.active) {
				PrepareTrack(from);
			}
		}
	}

	void AnimationComponent::CacheRestRootModelTranslation()
	{
		restRootValid = false;
		if (!skeleton) return;

		const int numSoa = skeleton->num_soa_joints();
		const int numJ   = skeleton->num_joints();
		if (numSoa <= 0 || numJ <= 0) return;

		// LTM of bind/rest pose → model-space hips (joint 0) position.
		std::vector<ozz::math::SoaTransform> restLocal(static_cast<size_t>(numSoa));
		const auto                           restSpan = skeleton->joint_rest_poses();
		for (int i = 0; i < numSoa; ++i) {
			restLocal[static_cast<size_t>(i)] = restSpan[i];
		}
		std::vector<ozz::math::Float4x4> restModel(static_cast<size_t>(numJ));
		ozz::animation::LocalToModelJob  ltm;
		ltm.skeleton = skeleton;
		ltm.input    = ozz::make_span(restLocal);
		ltm.output   = ozz::make_span(restModel);
		if (!ltm.Run()) return;

		float t[4];
		ozz::math::StorePtrU(restModel[0].cols[3], t);
		restRootX     = t[0];
		restRootY     = t[1];
		restRootZ     = t[2];
		restRootValid = true;
	}

	void AnimationComponent::ApplyRootOffsetCounteract()
	{
		if (!counteractRootOffset || !restRootValid || !model_pose || model_pose->empty() || !skeleton) {
			return;
		}
		if (static_cast<int>(model_pose->size()) < skeleton->num_joints()) {
			return;
		}

		// Animated root (hips) model translation.
		float rt[4] = {0.f, 0.f, 0.f, 1.f};
		ozz::math::StorePtrU((*model_pose)[0].cols[3], rt);
		if (!std::isfinite(rt[0]) || !std::isfinite(rt[1]) || !std::isfinite(rt[2])) {
			return;
		}

		// Delta from bind pose — base/root offset baked into the clip (e.g. Mixamo
		// climb_down floating high). Subtract from every joint so the mesh drops
		// back onto the capsule without changing relative limb pose.
		const float dx = rt[0] - restRootX;
		const float dy = rt[1] - restRootY;
		const float dz = rt[2] - restRootZ;
		if (!std::isfinite(dx) || !std::isfinite(dy) || !std::isfinite(dz)) {
			return;
		}
		if (dx * dx + dy * dy + dz * dz < 1e-12f) {
			return;
		}

		for (ozz::math::Float4x4& m : *model_pose) {
			float t[4] = {0.f, 0.f, 0.f, 1.f};
			ozz::math::StorePtrU(m.cols[3], t);
			if (!std::isfinite(t[0]) || !std::isfinite(t[1]) || !std::isfinite(t[2])) {
				continue;
			}
			t[0] -= dx;
			t[1] -= dy;
			t[2] -= dz;
			t[3] = 1.f;
			m.cols[3] = ozz::math::simd_float4::LoadPtrU(t);
		}
	}

	void AnimationComponent::ClearCrossfade()
	{
		from.Deactivate();
		isFading    = false;
		fadeElapsed = 0.f;
		blendWeight = 1.f;
	}

	void AnimationComponent::Play(const AnimationHandle& clip, bool loop, float fade)
	{
		if (!clip.IsValid()) {
			spdlog::error("AnimationComponent::Play: invalid clip");
			return;
		}
		if (!skeleton) {
			spdlog::error("AnimationComponent::Play: no skeleton loaded");
			return;
		}

		EnsurePoseBuffers();

		const float useFade = (fade < 0.f) ? defaultFadeDuration : fade;

		// Same clip already current — just update loop / optional restart from start if stopped
		if (current.active && current.clip == clip) {
			current.looping = loop;
			if (!IsPlaying()) {
				current.time = 0.f;
			}
			ClearCrossfade();
			return;
		}

		const bool canCrossfade = useFade > 0.f && current.active && current.clip.IsValid();

		if (canCrossfade) {
			// Snapshot current into `from` (re-prepare sampling buffers; Context is not movable).
			from.clip    = current.clip;
			from.time    = current.time;
			from.speed   = current.speed;
			from.looping = current.looping;
			if (!PrepareTrack(from)) {
				from.Deactivate();
			}

			current.clip    = clip;
			current.looping = loop;
			current.speed   = 1.f;
			current.time    = 0.f;
			if (!PrepareTrack(current)) {
				current.Deactivate();
				from.Deactivate();
				return;
			}

			isFading     = from.active;
			fadeDuration = useFade;
			fadeElapsed  = 0.f;
			blendWeight  = isFading ? 0.f : 1.f;
		} else {
			ClearCrossfade();
			current.clip    = clip;
			current.looping = loop;
			current.speed   = 1.f;
			current.time    = 0.f;
			if (!PrepareTrack(current)) {
				current.Deactivate();
				return;
			}
			blendWeight = 1.f;
		}
	}

	void AnimationComponent::Play(const std::string& path, bool loop, float fade)
	{
		Play(GetAssetManager().Load<Animation>(path), loop, fade);
	}

	void AnimationComponent::Stop()
	{
		current.Deactivate();
		ClearCrossfade();
	}

	void AnimationComponent::Seek(float timeSeconds)
	{
		current.time = std::max(0.f, timeSeconds);
		if (!current.active) return;

		Animation* clip = ResolveClip(current);
		if (!clip) return;

		const float dur = clip->Duration();
		if (dur <= 0.f) return;

		if (current.looping) {
			current.time = std::fmod(current.time, dur);
			if (current.time < 0.f) current.time += dur;
		} else {
			current.time = std::min(current.time, dur);
		}
	}

	bool AnimationComponent::IsPlaying() const
	{
		return current.active && current.clip.IsValid();
	}

	float AnimationComponent::GetLength() const
	{
		Animation* clip = ResolveClip(current);
		return clip ? clip->Duration() : 0.f;
	}

	bool AnimationComponent::HasSkeleton() const
	{ return skeleton != nullptr; }

	int AnimationComponent::JointCount() const
	{ return skeleton ? skeleton->num_joints() : 0; }

	void AnimationComponent::AdvanceTrack(AnimationTrack& track, float dt)
	{
		if (!track.active) return;

		Animation* clip = ResolveClip(track);
		if (!clip) return;

		const float duration = clip->Duration();
		if (duration <= 0.f) return;

		track.time += dt * track.speed * playbackSpeed;
		if (track.looping) {
			track.time = std::fmod(track.time, duration);
			if (track.time < 0.f) track.time += duration;
		} else {
			track.time = std::clamp(track.time, 0.f, duration);
		}
	}

	void AnimationComponent::SampleTrack(AnimationTrack& track)
	{
		if (!track.active || !skeleton) return;

		Animation* clip = ResolveClip(track);
		if (!clip) return;

		ozz::animation::Animation* ozzAnim = clip->Runtime();
		const float                duration = clip->Duration();
		const float                ratio    = duration > 0.f ? (track.time / duration) : 0.f;

		if ((int)track.localPose.size() != skeleton->num_soa_joints()) {
			track.localPose.resize(skeleton->num_soa_joints());
		}

		ozz::animation::SamplingJob job;
		job.animation = ozzAnim;
		job.context   = &track.context;
		job.ratio     = ratio;
		job.output    = ozz::make_span(track.localPose);
		if (!job.Run()) {
			GetDefaultLogger()->error("Animation sampling failed");
		}
	}

	void AnimationComponent::UpdatePlayback(float dt)
	{
		if (!skeleton || !local_pose || !model_pose) return;
		if (!current.active) return;

		// Crossfade progress (wall-clock, not scaled by clip speed)
		if (isFading) {
			if (fadeDuration <= 0.f) {
				blendWeight = 1.f;
				ClearCrossfade();
			} else {
				fadeElapsed += dt;
				blendWeight = std::clamp(fadeElapsed / fadeDuration, 0.f, 1.f);
				if (blendWeight >= 1.f) {
					ClearCrossfade();
				}
			}
		}

		AdvanceTrack(current, dt);
		if (from.active) {
			AdvanceTrack(from, dt);
		}

		EvaluatePose();
	}

	void AnimationComponent::EvaluatePose()
	{
		if (!skeleton || !local_pose || !model_pose) return;
		if (!current.active || !ResolveClip(current)) return;

		SampleTrack(current);

		const bool blendFrom =
		    isFading && from.active && ResolveClip(from) && blendWeight < 1.f - 1e-4f;

		if (!blendFrom || blendWeight >= 1.f) {
			std::copy(current.localPose.begin(), current.localPose.end(), local_pose->begin());
		} else if (blendWeight <= 1e-4f) {
			SampleTrack(from);
			std::copy(from.localPose.begin(), from.localPose.end(), local_pose->begin());
		} else {
			SampleTrack(from);

			ozz::animation::BlendingJob::Layer layers[2];
			layers[0].transform = ozz::make_span(from.localPose);
			layers[0].weight    = 1.f - blendWeight;
			layers[1].transform = ozz::make_span(current.localPose);
			layers[1].weight    = blendWeight;

			ozz::animation::BlendingJob blend;
			blend.threshold = ozz::animation::BlendingJob().threshold;
			blend.layers    = ozz::span<const ozz::animation::BlendingJob::Layer>(layers, 2);
			blend.rest_pose = skeleton->joint_rest_poses();
			blend.output    = ozz::make_span(*local_pose);
			if (!blend.Run()) {
				GetDefaultLogger()->error("Animation blend failed");
				return;
			}
		}

		ozz::animation::LocalToModelJob ltm;
		ltm.skeleton = skeleton;
		ltm.input    = ozz::make_span(*local_pose);
		ltm.output   = ozz::make_span(*model_pose);
		if (!ltm.Run()) {
			GetDefaultLogger()->error("LocalToModel failed");
			return;
		}

		ApplyRootOffsetCounteract();
	}

	void AnimationComponent::AddBindings()
	{
		auto& lua = GetScriptManager().lua;

		lua.new_usertype<AnimationComponent>(
		    "AnimationComponent",
		    "skeletonPath",
		    &AnimationComponent::skeletonPath,
		    "defaultFadeDuration",
		    &AnimationComponent::defaultFadeDuration,
		    "playbackSpeed",
		    &AnimationComponent::playbackSpeed,
		    "counteractRootOffset",
		    &AnimationComponent::counteractRootOffset,
		    "setSkeleton",
		    &AnimationComponent::SetSkeleton,

		    "play",
		    sol::overload(
		        [](AnimationComponent& self, const std::string& path) {
			        self.Play(path, true, -1.f);
		        },
		        [](AnimationComponent& self, const std::string& path, bool loop) {
			        self.Play(path, loop, -1.f);
		        },
		        [](AnimationComponent& self, const std::string& path, bool loop, float fade) {
			        self.Play(path, loop, fade);
		        },
		        [](AnimationComponent& self, const AnimationHandle& h) {
			        self.Play(h, true, -1.f);
		        },
		        [](AnimationComponent& self, const AnimationHandle& h, bool loop) {
			        self.Play(h, loop, -1.f);
		        },
		        [](AnimationComponent& self, const AnimationHandle& h, bool loop, float fade) {
			        self.Play(h, loop, fade);
		        }),

		    // Alias: crossfadeTo(path [, fade]) always loops
		    "crossfadeTo",
		    sol::overload(
		        [](AnimationComponent& self, const std::string& path) {
			        self.Play(path, true, self.defaultFadeDuration);
		        },
		        [](AnimationComponent& self, const std::string& path, float fade) {
			        self.Play(path, true, fade);
		        },
		        [](AnimationComponent& self, const AnimationHandle& h) {
			        self.Play(h, true, self.defaultFadeDuration);
		        },
		        [](AnimationComponent& self, const AnimationHandle& h, float fade) {
			        self.Play(h, true, fade);
		        }),

		    "stop",
		    &AnimationComponent::Stop,
		    "seek",
		    &AnimationComponent::Seek,
		    "isPlaying",
		    &AnimationComponent::IsPlaying,
		    "isFading",
		    &AnimationComponent::IsFading,
		    "getTime",
		    &AnimationComponent::GetTime,
		    "getLength",
		    &AnimationComponent::GetLength,
		    "getBlendWeight",
		    &AnimationComponent::GetBlendWeight,
		    "getCurrentClip",
		    &AnimationComponent::GetCurrentClip,
		    "hasSkeleton",
		    &AnimationComponent::HasSkeleton,
		    "jointCount",
		    &AnimationComponent::JointCount,
		    "speed",
		    sol::property(&AnimationComponent::GetSpeed, &AnimationComponent::SetSpeed),
		    "looping",
		    sol::property(&AnimationComponent::GetLooping, &AnimationComponent::SetLooping));
	}

	void AnimationComponent::RenderInspector(Entity& entity)
	{
		ImGui::Text("Skeleton: %s", skeleton ? "Loaded" : "None");
		ImGui::Text("Joints: %d", JointCount());
		LeftLabelSliderFloat("Playback Speed", &playbackSpeed, 0.f, 3.f);
		LeftLabelSliderFloat("Default Fade", &defaultFadeDuration, 0.f, 2.f);
		LeftLabelCheckbox("Counteract Root Offset", &counteractRootOffset);
		if (restRootValid) {
			ImGui::Text("Rest root: (%.2f, %.2f, %.2f)", restRootX, restRootY, restRootZ);
		}

		ImGui::Separator();
		ImGui::Text("Current Clip");
		AnimationHandle clip = current.clip;
		if (LeftLabelAssetAnimation("Animation", &clip)) {
			Play(clip, current.looping, 0.f);
		}

		bool loop = current.looping;
		if (LeftLabelCheckbox("Looping", &loop)) {
			current.looping = loop;
		}

		float speed = current.speed;
		if (LeftLabelSliderFloat("Clip Speed", &speed, 0.f, 3.f)) {
			current.speed = speed;
		}

		const float length = GetLength();
		float       t      = current.time;
		if (length > 0.f && LeftLabelSliderFloat("Time", &t, 0.f, length)) {
			Seek(t);
			EvaluatePose();
		}

		ImGui::Text("Playing: %s  Fading: %s  Blend: %.2f",
		            IsPlaying() ? "yes" : "no",
		            isFading ? "yes" : "no",
		            blendWeight);

		if (ImGui::Button("Stop")) {
			Stop();
		}
		ImGui::SameLine();
		if (ImGui::Button("Restart") && current.clip.IsValid()) {
			Play(current.clip, current.looping, 0.f);
		}
	}

} // namespace Engine::Components
