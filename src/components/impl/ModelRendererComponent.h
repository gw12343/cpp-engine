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

namespace Engine::Components {
	// Renderer component for 3D models
	class ModelRenderer : public Component {
	  public:
		AssetHandle<Rendering::Model> model;
		bool                          visible = true;

		ModelRenderer() = default;

		explicit ModelRenderer(const AssetHandle<Rendering::Model>& handle) : model(handle) {}
		// Draw the model with the given shader and transform
		void Draw(const Shader& shader, const Components::Transform& transform) const
		{
			if (visible && model.IsValid()) {
				const auto* actualModel = GetAssetManager().Get(model);
				if (!actualModel) return;

				// Set model matrix in shader
				shader.Bind();
				glm::mat4 modelMatrix = transform.GetMatrix();
				shader.SetMat4("model", &modelMatrix);

				// Draw the model
				actualModel->Draw(shader);
			}
		}

		void SetModel(const std::string& path);

		static void AddBindings();

		void OnAdded(Entity& entity) override;
		void RenderInspector(Entity& entity) override;
	};
} // namespace Engine::Components

#include "assets/AssetManager.inl"

#endif // CPP_ENGINE_MODELRENDERERCOMPONENT_H
