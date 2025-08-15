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
		AssetHandle<Rendering::Model> model;
		bool                          visible = true;

		ModelRenderer() = default;

		template <class Archive>
		void serialize(Archive& ar)
		{
			ar(cereal::make_nvp("visible", visible), cereal::make_nvp("model", model) // only save GUID
			);
		}

		explicit ModelRenderer(const AssetHandle<Rendering::Model>& handle) : model(handle) {}
		// Draw the model with the given shader and transform
		void Draw(const Shader& shader, const Components::Transform& transform) const;

		void SetModel(const std::string& path);

		static void AddBindings();

		void OnAdded(Entity& entity) override;
		void RenderInspector(Entity& entity) override;
	};
} // namespace Engine::Components


#endif // CPP_ENGINE_MODELRENDERERCOMPONENT_H
