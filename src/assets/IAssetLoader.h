//
// Created by gabe on 6/30/25.
//

#ifndef CPP_ENGINE_IASSETLOADER_H
#define CPP_ENGINE_IASSETLOADER_H

#include <memory>
#include <string>
namespace Engine {
	template <typename T>
	class IAssetLoader {
	  public:
		virtual ~IAssetLoader()                                          = default;
		virtual std::unique_ptr<T> LoadFromFile(const std::string& path) = 0;
	};
} // namespace Engine

#endif // CPP_ENGINE_IASSETLOADER_H
