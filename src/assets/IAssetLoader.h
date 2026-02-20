//
// Created by gabe on 6/30/25.
//

#ifndef CPP_ENGINE_IASSETLOADER_H
#define CPP_ENGINE_IASSETLOADER_H



namespace Engine {
	template <typename T>
	class IAssetLoader {
	  public:
		virtual ~IAssetLoader()                                          = default;
		virtual std::unique_ptr<T> LoadFromFile(const std::string& path) = 0;
		virtual bool               Reload(T& asset, const std::string& path) { return false; }
	};
} // namespace Engine

#endif // CPP_ENGINE_IASSETLOADER_H
