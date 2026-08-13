#include "SkeletonLoader.h"

#include "animation/AnimationUtils.h"
#include "core/EngineData.h"

namespace Engine {

	std::unique_ptr<Skeleton> SkeletonLoader::LoadFromFile(const std::string& path)
	{
		auto skeleton = std::make_unique<Skeleton>();
		auto* ozzSkel = new ozz::animation::Skeleton();
		if (!LoadSkeleton(path.c_str(), ozzSkel)) {
			spdlog::error("SkeletonLoader: failed to load skeleton from path: {}", path);
			delete ozzSkel;
			return nullptr;
		}

		skeleton->name   = path.substr(path.find_last_of("/\\") + 1);
		skeleton->source = ozzSkel;
		GetDefaultLogger()->info("Loaded skeleton from path: {} ({} joints)", path, ozzSkel->num_joints());
		return skeleton;
	}

} // namespace Engine
