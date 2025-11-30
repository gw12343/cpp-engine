//
// Created by gabe on 6/30/25.
//


#ifndef CPP_ENGINE_TEXTURELOADER_H
#define CPP_ENGINE_TEXTURELOADER_H

#include "assets/IAssetLoader.h"
#include "rendering/Texture.h"

namespace Engine {


	class TextureLoader : public IAssetLoader<Texture> {
	  public:
		std::unique_ptr<Texture> LoadFromFile(const std::string& path) override;
		bool                     Reload(Texture& asset, const std::string& path) override;
	};
} // namespace Engine

#endif // CPP_ENGINE_TEXTURELOADER_H
