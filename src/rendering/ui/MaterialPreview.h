//
// Created by gabe on 11/23/25.
//

#ifndef CPP_ENGINE_MATERIALPREVIEW_H
#define CPP_ENGINE_MATERIALPREVIEW_H

#include "rendering/Material.h"
#include "rendering/Shader.h"

#define MATERIAL_PREVIEW_SIZE 128

namespace Engine {
	struct MaterialPreview {
		unsigned int fbo         = 0;
		unsigned int texture     = 0;
		unsigned int depth       = 0;
		unsigned int sphereVAO   = 0;
		unsigned int sphereVBO   = 0;
		unsigned int sphereEBO   = 0;
		int          indexCount  = 0;
		int          width       = MATERIAL_PREVIEW_SIZE;
		int          height      = MATERIAL_PREVIEW_SIZE;
		bool         initialized = false;

		void Initialize();
		void Render(Material* material, Shader& shader);
		void Render(Material* material, Shader& shader, float yaw, float pitch);

	  private:
		void GenerateSphere();
	};
} // namespace Engine

#endif // CPP_ENGINE_MATERIALPREVIEW_H
