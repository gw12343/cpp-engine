#include "AnimationManager.h"

#include "AnimationUtils.h"
#include "Animation.h"
#include "Skeleton.h"
#include "SkinnedMeshCache.h"
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
#include "core/ThreadPool.h"
#include <algorithm>
#include <cmath>
#include <tracy/Tracy.hpp>
#include <vector>

namespace Engine {

	void AnimationManager::setLuaBindings()
	{
		auto& lua = GetScriptManager().lua;

		lua.new_usertype<AnimationManager>(
		    "AnimationManager",
		    "getDrawSkeleton",
		    &AnimationManager::GetDrawSkeletonValue,
		    "setDrawSkeleton",
		    &AnimationManager::SetDrawSkeleton,
		    "getDrawMesh",
		    &AnimationManager::GetDrawMeshValue,
		    "setDrawMesh",
		    &AnimationManager::SetDrawMesh,
		    "drawSkeleton",
		    sol::property(&AnimationManager::GetDrawSkeletonValue, &AnimationManager::SetDrawSkeleton),
		    "drawMesh",
		    sol::property(&AnimationManager::GetDrawMeshValue, &AnimationManager::SetDrawMesh));

		lua.set_function("getAnimationManager", []() -> AnimationManager& { return Engine::GetAnimationManager(); });

		lua.set_function("loadAnimation", [](const std::string& path) -> AnimationHandle { return GetAssetManager().Load<Animation>(path); });
		lua.set_function("loadSkeleton", [](const std::string& path) -> SkeletonReference { return GetAssetManager().Load<Skeleton>(path); });
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
		}
		else {
			log->info("Initialized animated renderer");
		}
	}

	void AnimationManager::onShutdown()
	{
		if (Get().assetManager) {
			GetAssetManager().UnloadAll<Animation>();
			GetAssetManager().UnloadAll<Skeleton>();
		}

		renderer_.reset();
		loaded_skeletons_.clear();
	}

	void AnimationManager::onUpdate(float deltaTime)
	{
		ZoneScopedN("Animation Update");

		// Collect independent AnimationComponents, then evaluate poses in parallel.
		// Each component owns its pose buffers — no shared writes.
		struct AnimWork {
			Components::AnimationComponent* ac = nullptr;
		};
		std::vector<AnimWork> work;
		work.reserve(32);

		auto animationView = GetCurrentSceneRegistry().view<Components::EntityMetadata, Components::AnimationComponent>();
		for (auto [entity, metadata, ac] : animationView.each()) {
			if (!metadata.active) continue;
			if (!ac.skeleton) continue;
			work.push_back(AnimWork{&ac});
		}

		const float dt      = (GetState() == PLAYING) ? deltaTime : 0.f;
		const int   n       = static_cast<int>(work.size());
		// Pose sampling is relatively heavy — parallelize even modest counts.
		GetThreadPool().ParallelForIndex(n, /*minPerTask=*/1, [&](int i) {
			ZoneScopedN("Anim Pose Entity");
			auto& ac = *work[static_cast<size_t>(i)].ac;
			if (ac.IsPlaying()) {
				ac.UpdatePlayback(dt);
			}
			else if (ac.local_pose && ac.model_pose) {
				ac.EvaluatePose();
			}
		});

		// Invalidate skinned caches for the upcoming render.
		++pose_generation_;
	}

	void AnimationManager::PrepareSkinnedMeshes()
	{
		ZoneScopedN("PrepareSkinnedMeshes");

		// Already skinned for this pose generation (shadow / GBuffer / pick share one build).
		if (skinned_generation_ == pose_generation_) {
			return;
		}
		skinned_generation_ = pose_generation_;

		struct EntitySkinWork {
			Components::SkinnedMeshComponent* skinned = nullptr;
			Components::AnimationComponent*   anim    = nullptr;
		};

		std::vector<EntitySkinWork> work;
		{
			ZoneScopedN("Collect Skinned Entities");
			auto view = GetCurrentSceneRegistry().view<Components::SkinnedMeshComponent, Components::AnimationComponent, Components::Transform>();
			work.reserve(32);
			for (auto entity : view) {
				Entity e(entity, GetCurrentScene());
				auto&  skinned = e.GetComponent<Components::SkinnedMeshComponent>();
				auto&  anim    = e.GetComponent<Components::AnimationComponent>();
				if (!anim.model_pose || !skinned.meshes || !skinned.skinning_matrices) {
					continue;
				}
				if (skinned.meshes->empty()) {
					continue;
				}
				// Ensure cache slots match mesh count
				if (skinned.skin_frame_cache.size() != skinned.meshes->size()) {
					skinned.skin_frame_cache.clear();
					skinned.skin_frame_cache.resize(skinned.meshes->size());
				}
				// Invalidate caches until rebuilt
				for (auto& c : skinned.skin_frame_cache) {
					c.valid = false;
				}
				skinned.skin_cache_frame = pose_generation_;
				work.push_back(EntitySkinWork{&skinned, &anim});
			}
		}

		if (work.empty()) {
			return;
		}

		// Parallel per-entity via global ThreadPool (each entity owns its buffers).
		const int entityCount = static_cast<int>(work.size());
		GetThreadPool().ParallelForIndex(entityCount, /*minPerTask=*/1, [&](int ei) {
			ZoneScopedN("Skin Entity");
			auto& skinned = *work[static_cast<size_t>(ei)].skinned;
			auto& anim    = *work[static_cast<size_t>(ei)].anim;

			for (size_t mi = 0; mi < skinned.meshes->size(); ++mi) {
				const AnimatedMesh& mesh = (*skinned.meshes)[mi];
				{
					ZoneScopedN("Build Skinning Matrices");
					for (size_t j = 0; j < mesh.joint_remaps.size(); ++j) {
						(*skinned.skinning_matrices)[j] = (*anim.model_pose)[mesh.joint_remaps[j]] * mesh.inverse_bind_poses[j];
					}
				}
				// Parts of a mesh also parallelize via ParallelForIndex → ThreadPool.
				SkinAnimatedMeshToCache(mesh, ozz::make_span(*skinned.skinning_matrices), skinned.skin_frame_cache[mi]);
			}
		});
	}

	void AnimationManager::Render()
	{
		ZoneScopedN("AnimationManager::Render (GBuffer)");
		PrepareSkinnedMeshes();

		auto view = GetCurrentSceneRegistry().view<Components::SkinnedMeshComponent, Components::AnimationComponent, Components::Transform>();
		for (auto entity : view) {
			ZoneScopedN("GBuffer Skinned Entity");
			Entity e(entity, GetCurrentScene());
			auto&  skinned = e.GetComponent<Components::SkinnedMeshComponent>();
			if (!skinned.visible) continue;
			if (!skinned.meshes || skinned.skin_frame_cache.empty()) continue;

			const ozz::math::Float4x4 transform = FromMatrix(e.GetComponent<Components::Transform>().GetWorldMatrix());

			for (size_t mi = 0; mi < skinned.meshes->size(); ++mi) {
				const auto& mesh  = (*skinned.meshes)[mi];
				const auto& cache = skinned.skin_frame_cache[mi];
				if (!cache.valid) continue;
				renderer_->DrawSkinnedMeshCached(cache, mesh, transform, skinned.meshMaterial, render_options_);
			}
		}
	}

	void AnimationManager::RenderDebug() const
	{
	}

	ozz::animation::Skeleton* AnimationManager::LoadSkeletonFromPath(const std::string& path)
	{
		// Prefer the asset system so GUIDs / inspector / scripts share one cache.
		const SkeletonReference handle = GetAssetManager().Load<Skeleton>(path);
		if (Skeleton* asset = GetAssetManager().Get(handle)) {
			return asset->Runtime();
		}
		log->error("Failed to load skeleton asset from path: {}", path);
		return nullptr;
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
