//
// Created by gabe on 8/23/25.
//

#ifndef CPP_ENGINE_MATERIALEDITOR_H
#define CPP_ENGINE_MATERIALEDITOR_H

#include "rendering/Material.h"
namespace Engine {
	class MaterialEditor {
	  public:
		void RenderMaterialEditor(AssetHandle<Material> matRef);
	};
} // namespace Engine
#endif // CPP_ENGINE_MATERIALEDITOR_H
