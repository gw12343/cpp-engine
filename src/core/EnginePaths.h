#pragma once

#include <string>

namespace Engine {

	// Engine install/working tree (contains resources/). Independent of the open project.
	class EnginePaths {
	  public:
		static EnginePaths& Get();

		void Detect();

		[[nodiscard]] const std::string& EngineRoot() const { return m_engineRoot; }
		[[nodiscard]] std::string        ResourcesDir() const;
		[[nodiscard]] std::string        ExecutableDirectory() const;

		// Logical paths (assets/..., scenes/..., scripts/..., resources/...)
		// map onto the project folder or the engine tree.
		[[nodiscard]] std::string Resolve(const std::string& path) const;
		[[nodiscard]] std::string ToLogical(const std::string& path) const;
		[[nodiscard]] bool        Exists(const std::string& path) const;

	  private:
		std::string m_engineRoot;
	};

	inline EnginePaths& GetEnginePaths() { return EnginePaths::Get(); }
	inline std::string  ResolvePath(const std::string& path) { return GetEnginePaths().Resolve(path); }

} // namespace Engine
