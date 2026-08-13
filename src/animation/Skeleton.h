#pragma once

#include "ozz/animation/runtime/skeleton.h"

#include <string>

namespace Engine {
	// Engine asset wrapper around an ozz runtime skeleton.
	// Prefer this type in engine code; only pass Runtime() into ozz jobs.
	class Skeleton {
	  public:
		Skeleton() = default;
		~Skeleton();

		Skeleton(const Skeleton&)            = delete;
		Skeleton& operator=(const Skeleton&) = delete;
		Skeleton(Skeleton&& other) noexcept;
		Skeleton& operator=(Skeleton&& other) noexcept;

		std::string name;
		// Owned ozz skeleton data (loaded from skeleton .ozz archives).
		ozz::animation::Skeleton* source = nullptr;

		[[nodiscard]] bool IsValid() const { return source != nullptr; }

		// ozz runtime object for LocalToModel / joint queries.
		[[nodiscard]] ozz::animation::Skeleton* Runtime() const { return source; }

		[[nodiscard]] int NumJoints() const
		{
			return source ? source->num_joints() : 0;
		}

		[[nodiscard]] int NumSoaJoints() const
		{
			return source ? source->num_soa_joints() : 0;
		}
	};
} // namespace Engine
