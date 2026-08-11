//
// AnimationComponent — Godot AnimationPlayer / Unity Animator (simple) style.
//
// Full-body playback with optional crossfade between two clips only.
// ozz handles sampling + blending under the hood.
//

#pragma once

#include "components/Components.h"
#include "animation/Animation.h"
#include "animation/AnimationPlayer.h"

#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/base/containers/vector.h>

#include <cereal/cereal.hpp>
#include <string>
#include <vector>

namespace Engine::Components {

	class AnimationComponent : public Component {
	  public:
		std::string skeletonPath;
		float       defaultFadeDuration = 0.2f;
		float       playbackSpeed       = 1.f; // global multiplier

		// When true, root (hips) model translation is forced back to the skeleton rest
		// position every frame — cancels Mixamo base/root height offsets (e.g. climb_down).
		// Enable from script for in-place climb clips; leave false for normal locomotion.
		bool counteractRootOffset = false;

		// Serialized playback state (current clip only — crossfade is runtime)
		AnimationTrack current{};

		// Runtime
		ozz::animation::Skeleton*             skeleton   = nullptr;
		std::vector<ozz::math::SoaTransform>* local_pose = nullptr;
		std::vector<ozz::math::Float4x4>*     model_pose = nullptr;

		AnimationTrack from{}; // previous clip while crossfading
		float          blendWeight = 1.f; // 0 = fully from, 1 = fully current
		float          fadeDuration = 0.2f;
		float          fadeElapsed  = 0.f;
		bool           isFading     = false;

		// Rest-pose hips/root model translation (cached when skeleton loads).
		bool      restRootValid = false;
		float     restRootX = 0.f, restRootY = 0.f, restRootZ = 0.f;

		AnimationComponent() = default;
		AnimationComponent(const AnimationComponent& other);
		AnimationComponent& operator=(const AnimationComponent& other);
		AnimationComponent(AnimationComponent&&)                 = default;
		AnimationComponent& operator=(AnimationComponent&&)      = default;

		template <class Archive>
		void serialize(Archive& ar)
		{
			ar(CEREAL_NVP(skeletonPath),
			   CEREAL_NVP(defaultFadeDuration),
			   CEREAL_NVP(playbackSpeed),
			   CEREAL_NVP(current));
		}

		void OnAdded(Entity& entity) override;
		void OnRemoved(Entity& entity) override;
		void RenderInspector(Entity& entity) override;

		void SetSkeleton(const std::string& path);

		// --- Playback API (Unity/Godot style) ---

		// Play clip immediately (fade=0) or crossfade from current.
		void Play(const AnimationHandle& clip, bool loop = true, float fade = -1.f);
		void Play(const std::string& path, bool loop = true, float fade = -1.f);

		void Stop();
		void Seek(float timeSeconds);
		bool IsPlaying() const;
		bool IsFading() const { return isFading; }

		float GetTime() const { return current.time; }
		float GetLength() const;
		float GetBlendWeight() const { return blendWeight; }

		AnimationHandle GetCurrentClip() const { return current.clip; }
		bool            HasSkeleton() const;
		int             JointCount() const;

		// Per-clip speed (current track)
		float GetSpeed() const { return current.speed; }
		void  SetSpeed(float s) { current.speed = s; }
		bool  GetLooping() const { return current.looping; }
		void  SetLooping(bool loop) { current.looping = loop; }

		// Called by AnimationManager each frame
		void UpdatePlayback(float dt);
		void EvaluatePose();

		static void AddBindings();
		static void CleanAnimationContexts();

	  private:
		void EnsurePoseBuffers();
		bool PrepareTrack(AnimationTrack& track);
		void CacheRestRootModelTranslation();
		void ApplyRootOffsetCounteract();

		// Resolve engine asset from a track's handle (null if missing/invalid).
		static Animation* ResolveClip(const AnimationTrack& track);

		// Sample track.localPose from its clip asset (uses clip->Runtime() for ozz).
		void SampleTrack(AnimationTrack& track);

		// Advance track.time using the clip asset's duration.
		void AdvanceTrack(AnimationTrack& track, float dt);

		void ClearCrossfade();
	};

} // namespace Engine::Components
