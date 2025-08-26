//
// Created by gabe on 8/26/25.
//

#ifndef CPP_ENGINE_ENTITYHANDLE_H
#define CPP_ENGINE_ENTITYHANDLE_H

#include <cstdint>
#include <string>
namespace Engine {
	class EntityHandle {
		std::string guid;

	  public:
		EntityHandle() = default;
		explicit EntityHandle(const std::string guid) : guid(guid) {}
		[[nodiscard]] const std::string& GetID() const { return guid; }
		[[nodiscard]] bool               IsValid() const { return !guid.empty(); }

		bool operator==(const EntityHandle& other) const { return guid == other.guid; }
		bool operator<(const EntityHandle& other) const { return guid < other.guid; }
	};
} // namespace Engine

#endif // CPP_ENGINE_ENTITYHANDLE_H
