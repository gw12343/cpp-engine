//
// Created by gabe on 6/30/25.
//

#ifndef CPP_ENGINE_MODELRENDERERCOMPONENT_H
#define CPP_ENGINE_MODELRENDERERCOMPONENT_H

#include "components/Components.h"
#include "rendering/Renderer.h"
#include "TransformComponent.h"
#include "assets/AssetManager.h"
#include "core/EngineData.h"

#include <cereal/cereal.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/array.hpp>

namespace Engine::Components {
	// Renderer component for 3D models
	class ModelRenderer : public Component {
	  public:
		AssetHandle<Rendering::Model>      model;
		bool                               visible         = true;
		bool                               backfaceCulling = true;
		std::vector<AssetHandle<Material>> materialOverrides;
		ModelRenderer() = default;

		template <class Archive>
		void serialize(Archive& ar)
		{
			ar(cereal::make_nvp("visible", visible), cereal::make_nvp("model", model), cereal::make_nvp("backfaceCulling", backfaceCulling), cereal::make_nvp("materialOverrides", materialOverrides) // only save GUID
			);
		}

		explicit ModelRenderer(const AssetHandle<Rendering::Model>& handle) : model(handle) {}
		// Draw the model with the given shader and transform
		void Draw(const Shader& shader, Components::Transform& transform, bool uploadMaterial);

		void SetModel(const std::string& path);
		void SetMaterial(AssetHandle<Material> mat);

		static void AddBindings();

		void OnAdded(Entity& entity) override;
		void OnRemoved(Entity& entity) override;
		void RenderInspector(Entity& entity) override;
	};
} // namespace Engine::Components


#endif // CPP_ENGINE_MODELRENDERERCOMPONENT_H
