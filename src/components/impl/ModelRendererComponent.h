//
// Created by gabe on 6/30/25.
//

#ifndef CPP_ENGINE_MODELRENDERERCOMPONENT_H
#define CPP_ENGINE_MODELRENDERERCOMPONENT_H

#include "components/Components.h"
#include "rendering/Renderer.h"
#include "TransformComponent.h"

namespace Engine::Components {
	// Renderer component for 3D models
	class ModelRenderer : public Component {
	  public:
		std::shared_ptr<Rendering::Model> model;
		bool                              visible = true;

		ModelRenderer() = default;

		explicit ModelRenderer(const std::shared_ptr<Rendering::Model>& model) : model(model) {}

		// Draw the model with the given shader and transform
		void Draw(const Shader& shader, const Components::Transform& transform) const
		{
			if (visible && model) {
				// Set model matrix in shader
				shader.Bind();
				glm::mat4 modelMatrix = transform.GetMatrix();
				shader.SetMat4("model", &modelMatrix);

				// Draw the model
				model->Draw(shader);
			}
		}

		void SetModel(std::string path);

		static void AddBindings();

		void OnAdded(Entity& entity) override;
		void RenderInspector(Entity& entity) override;
	};
} // namespace Engine::Components


#endif // CPP_ENGINE_MODELRENDERERCOMPONENT_H
