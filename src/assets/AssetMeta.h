//
// Created by gabe on 7/6/25.
//

#ifndef CPP_ENGINE_ASSETMETA_H
#define CPP_ENGINE_ASSETMETA_H

#include <string>

namespace Engine {
	struct AssetMeta {
		std::string guid;
		std::string path; // canonical / normalized
		std::string type;
		time_t      lastWriteTime;
	};
} // namespace Engine

#endif // CPP_ENGINE_ASSETMETA_H
