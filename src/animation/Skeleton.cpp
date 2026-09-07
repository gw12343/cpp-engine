#include "Skeleton.h"

#include "ozz/animation/runtime/skeleton.h"

namespace Engine {

	Skeleton::~Skeleton()
	{
		delete source;
		source = nullptr;
	}

	Skeleton::Skeleton(Skeleton&& other) noexcept
	    : name(std::move(other.name))
	    , source(other.source)
	{
		other.source = nullptr;
	}

	Skeleton& Skeleton::operator=(Skeleton&& other) noexcept
	{
		if (this != &other) {
			delete source;
			name         = std::move(other.name);
			source       = other.source;
			other.source = nullptr;
		}
		return *this;
	}

	int Skeleton::NumJoints() const
	{
		return source ? source->num_joints() : 0;
	}

	int Skeleton::NumSoaJoints() const
	{
		return source ? source->num_soa_joints() : 0;
	}

} // namespace Engine
