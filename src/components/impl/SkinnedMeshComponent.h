//
// Created by gabe on 6/30/25.
//

#ifndef CPP_ENGINE_SKINNEDMESHCOMPONENT_H
#define CPP_ENGINE_SKINNEDMESHCOMPONENT_H

#include <animation/rendering/AnimatedMesh.h>
#include <animation/SkinnedMeshCache.h>

#include "components/Components.h"
#include "rendering/Renderer.h"

#include "ozz/base/containers/vector.h"
#include <cereal/cereal.hpp>

namespace Engine::Components {
	class SkinnedMeshComponent : public Component {
	  public:
		ozz::vector<Engine::AnimatedMesh>*        meshes            = nullptr;
		std::vector<ozz::math::Float4x4>* skinning_matrices = nullptr;
		/// One CPU-skinned VBO cache per mesh, rebuilt once per frame.
		std::vector<SkinnedMeshFrameCache> skin_frame_cache;
		uint64_t                           skin_cache_frame = 0;
		std::string                       meshPath;
        bool visible = true;

		MaterialHandle meshMaterial;

		SkinnedMeshComponent() = default;

		template <class Archive>
		void serialize(Archive& ar)
		{
			ar(CEREAL_NVP(visible), CEREAL_NVP(meshPath), CEREAL_NVP(meshMaterial));
		}


		explicit SkinnedMeshComponent(ozz::vector<Engine::AnimatedMesh>* meshes) : meshes(meshes) {}
		explicit SkinnedMeshComponent(std::string meshPath) : meshPath(std::move(meshPath)) {}

		void OnAdded(Entity& entity) override;
		void OnRemoved(Entity& entity) override;
		void RenderInspector(Entity& entity) override;

		/** Load (or reload) meshes from meshPath. Safe if path is empty or load fails. */
		void SetMeshPath(const std::string& path);
		bool TryLoadMeshes();

		static void CleanSkinnedModels();

		static std::unordered_set<std::vector<ozz::math::Float4x4>*> s_skin_mats;
		static std::unordered_set<ozz::vector<Engine::AnimatedMesh>*>        s_all_meshes;

	  private:
		void FreeMeshes();
		void FreeSkinningMatrices();
		void RebuildSkinningMatrices();
	};
} // namespace Engine::Components

#endif // CPP_ENGINE_SKINNEDMESHCOMPONENT_H
