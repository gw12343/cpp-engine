#include "Skeleton.h"

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

} // namespace Engine
