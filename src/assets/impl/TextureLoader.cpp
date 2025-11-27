//
// Created by gabe on 6/30/25.
//

#include "TextureLoader.h"


#include <iostream>

namespace Engine {
	std::unique_ptr<Texture> TextureLoader::LoadFromFile(const std::string& path)
	{
		auto tex = std::make_unique<Texture>();
		tex->LoadFromFile(path);

		return tex;
	}
	
	bool TextureLoader::Reload(Texture& asset, const std::string& path)
	{
		return asset.LoadFromFile(path);
	}

} // namespace Engine