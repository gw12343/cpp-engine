//
// Created by gabe on 6/30/25.
//

#ifndef CPP_ENGINE_SKINNEDMESHCOMPONENT_H
#define CPP_ENGINE_SKINNEDMESHCOMPONENT_H

#include "components/Components.h"
#include "rendering/Renderer.h"
#include "TransformComponent.h"
#include "assets/AssetManager.h"
#include "core/EngineData.h"

#include <cereal/cereal.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/array.hpp>

namespace Engine::Components {
	class SkinnedMeshComponent : public Component {
	  public:
		ozz::vector<Engine::Mesh>*        meshes            = nullptr;
		std::vector<ozz::math::Float4x4>* skinning_matrices = nullptr;
		std::string                       meshPath;

		AssetHandle<Material> meshMaterial;

		SkinnedMeshComponent() = default;

		template <class Archive>
		void serialize(Archive& ar)
		{
			ar(CEREAL_NVP(meshPath), CEREAL_NVP(meshMaterial));
		}


		explicit SkinnedMeshComponent(ozz::vector<Engine::Mesh>* meshes) : meshes(meshes) {}
		explicit SkinnedMeshComponent(std::string meshPath) : meshPath(std::move(meshPath)) {}

		void OnAdded(Entity& entity) override;
		void OnRemoved(Entity& entity) override;
		void RenderInspector(Entity& entity) override;

		static void CleanSkinnedModels();

		static std::unordered_set<std::vector<ozz::math::Float4x4>*> s_skin_mats;
		static std::unordered_set<ozz::vector<Engine::Mesh>*>        s_all_meshes;
	};
} // namespace Engine::Components

#endif // CPP_ENGINE_SKINNEDMESHCOMPONENT_H
