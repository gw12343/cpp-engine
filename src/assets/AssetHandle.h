//
// Created by gabe on 6/30/25.
//

#ifndef CPP_ENGINE_ASSETHANDLE_H
#define CPP_ENGINE_ASSETHANDLE_H

#include <cstdint>
namespace Engine {
	template <typename T>
	class AssetHandle {
		std::string guid;

	  public:
		AssetHandle() = default;
		explicit AssetHandle(const std::string& guid) : guid(guid) {}
		const std::string& GetID() const { return guid; }
		bool               IsValid() const { return !guid.empty(); }

		bool operator==(const AssetHandle<T>& other) const { return guid == other.guid; }
	};
} // namespace Engine

#endif // CPP_ENGINE_ASSETHANDLE_H
