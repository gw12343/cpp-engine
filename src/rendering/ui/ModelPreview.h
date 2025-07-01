//
// Created by gabe on 7/1/25.
//

#ifndef CPP_ENGINE_MODELPREVIEW_H
#define CPP_ENGINE_MODELPREVIEW_H


#include "rendering/Model.h"

#define MODEL_PREVIEW_SIZE 128

namespace Engine {
	struct ModelPreview {
		GLuint fbo         = 0;
		GLuint texture     = 0;
		GLuint depth       = 0;
		int    width       = MODEL_PREVIEW_SIZE;
		int    height      = MODEL_PREVIEW_SIZE;
		bool   initialized = false;

		void Initialize();
		void Render(Rendering::Model* model, Shader& shader);
	};
} // namespace Engine

#endif // CPP_ENGINE_MODELPREVIEW_H
