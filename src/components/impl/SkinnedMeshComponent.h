//
// Created by gabe on 6/30/25.
//

#ifndef CPP_ENGINE_SKINNEDMESHCOMPONENT_H
#define CPP_ENGINE_SKINNEDMESHCOMPONENT_H

#include "components/Components.h"

namespace Engine::Components {
	class SkinnedMeshComponent : public Component {
	  public:
		ozz::vector<Engine::Mesh>*        meshes            = nullptr;
		std::vector<ozz::math::Float4x4>* skinning_matrices = nullptr;
		std::string                       meshPath;

		SkinnedMeshComponent() = default;
		explicit SkinnedMeshComponent(ozz::vector<Engine::Mesh>* meshes) : meshes(meshes) {}
		explicit SkinnedMeshComponent(std::string meshPath) : meshPath(std::move(meshPath)) {}

		void OnAdded(Entity& entity) override;
		void RenderInspector(Entity& entity) override;

		static void CleanSkinnedModels();

		static std::unordered_set<std::vector<ozz::math::Float4x4>*> s_skin_mats;
		static std::unordered_set<ozz::vector<Engine::Mesh>*>        s_all_meshes;
	};
} // namespace Engine::Components

#endif // CPP_ENGINE_SKINNEDMESHCOMPONENT_H
