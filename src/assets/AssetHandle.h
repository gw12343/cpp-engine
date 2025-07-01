//
// Created by gabe on 6/30/25.
//

#ifndef CPP_ENGINE_ASSETHANDLE_H
#define CPP_ENGINE_ASSETHANDLE_H

#include <cstdint>
namespace Engine {
	template <typename T>
	class AssetHandle {
		uint32_t id = 0; // or hash, or UUID
	  public:
		constexpr AssetHandle() = default;
		explicit AssetHandle(uint32_t id) : id(id) {}
		[[nodiscard]] uint32_t GetID() const { return id; }

		[[nodiscard]] bool IsValid() const { return id != 0; }
		bool               operator==(const AssetHandle<T>& other) const { return id == other.id; }
	};
} // namespace Engine

#endif // CPP_ENGINE_ASSETHANDLE_H
