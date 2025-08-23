//
// Created by gabe on 8/22/25.
//

#ifndef CPP_ENGINE_MATERIALLOADER_H
#define CPP_ENGINE_MATERIALLOADER_H

#include "assets/IAssetLoader.h"
#include "rendering/Material.h"

namespace Engine {


	class MaterialLoader : public IAssetLoader<Material> {
	  public:
		std::unique_ptr<Material> LoadFromFile(const std::string& path) override;
		void                      SaveMaterial(const Material& mat, const std::string& path);
	};
} // namespace Engine

#endif // CPP_ENGINE_MATERIALLOADER_H
