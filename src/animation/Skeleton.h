#pragma once

#include <string>

namespace ozz {
	namespace animation {
		class Skeleton;
	}
} // namespace ozz

namespace Engine {
	class Skeleton {
	  public:
		Skeleton() = default;
		~Skeleton();

		Skeleton(const Skeleton&)            = delete;
		Skeleton& operator=(const Skeleton&) = delete;
		Skeleton(Skeleton&& other) noexcept;
		Skeleton& operator=(Skeleton&& other) noexcept;

		std::string name;
		ozz::animation::Skeleton* source = nullptr;

		[[nodiscard]] bool IsValid() const { return source != nullptr; }

		[[nodiscard]] ozz::animation::Skeleton* Runtime() const { return source; }

		[[nodiscard]] int NumJoints() const;
		[[nodiscard]] int NumSoaJoints() const;
	};
} // namespace Engine
