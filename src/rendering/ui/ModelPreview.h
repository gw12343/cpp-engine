//
// Created by gabe on 7/1/25.
//

#ifndef CPP_ENGINE_MODELPREVIEW_H
#define CPP_ENGINE_MODELPREVIEW_H

#include "rendering/ui/PreviewDrawItem.h"

#include <vector>

#define MODEL_PREVIEW_SIZE 128

namespace Engine {
	class Shader;
	namespace Rendering {
		class Model;
	}

	struct ModelPreview {
		unsigned int fbo         = 0;
		unsigned int texture     = 0;
		unsigned int depth       = 0;
		int          width       = MODEL_PREVIEW_SIZE;
		int          height      = MODEL_PREVIEW_SIZE;
		bool         initialized = false;

		void Initialize();
		void Render(Rendering::Model* model, Shader& shader);
		void Render(const std::vector<PreviewDrawItem>& items, Shader& shader);
	};
} // namespace Engine

#endif // CPP_ENGINE_MODELPREVIEW_H
