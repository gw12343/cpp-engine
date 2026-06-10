//
// Created by gabe on 6/30/25.
//

#ifndef CPP_ENGINE_SKINNEDMESHCOMPONENT_H
#define CPP_ENGINE_SKINNEDMESHCOMPONENT_H

#include <animation/rendering/AnimatedMesh.h>

#include "components/Components.h"
#include "rendering/Renderer.h"

#include "ozz/base/containers/vector.h"
#include <cereal/cereal.hpp>

namespace Engine::Components {
	class SkinnedMeshComponent : public Component {
	  public:
		ozz::vector<Engine::AnimatedMesh>*        meshes            = nullptr;
		std::vector<ozz::math::Float4x4>* skinning_matrices = nullptr;
		std::string                       meshPath;
        bool visible = true;

		AssetHandle<Material> meshMaterial;

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

		static void CleanSkinnedModels();

		static std::unordered_set<std::vector<ozz::math::Float4x4>*> s_skin_mats;
		static std::unordered_set<ozz::vector<Engine::AnimatedMesh>*>        s_all_meshes;
	};
} // namespace Engine::Components

#endif // CPP_ENGINE_SKINNEDMESHCOMPONENT_H
