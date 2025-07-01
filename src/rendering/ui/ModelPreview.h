//
// Created by gabe on 7/1/25.
//

#ifndef CPP_ENGINE_MODELPREVIEW_H
#define CPP_ENGINE_MODELPREVIEW_H


#include "rendering/Model.h"

namespace Engine {
	struct ModelPreview {
		GLuint fbo         = 0;
		GLuint texture     = 0;
		GLuint depth       = 0;
		int    width       = 128;
		int    height      = 128;
		bool   initialized = false;

		void Initialize();
		void Render(Rendering::Model* model, Shader& shader, float size);
	};
} // namespace Engine

#endif // CPP_ENGINE_MODELPREVIEW_H
