#pragma once

#include "animation/rendering/AnimatedMesh.h"

#include "ozz/base/maths/simd_math.h"
#include "ozz/base/span.h"

#include <cstdint>
#include <functional>
#include <vector>

namespace Engine {

	/// CPU-skinned vertex buffer for one AnimatedMesh, shared by shadow / GBuffer / pick draws.
	/// Layout matches the sample skinned renderer packing:
	///   [positions xyz][normals xyz][tangents xyz][colors rgba u8][uvs xy]
	struct SkinnedMeshFrameCache {
		std::vector<uint8_t> vbo;
		int                  vertex_count = 0;
		bool                 valid        = false;

		[[nodiscard]] int positionsOffset() const { return 0; }
		[[nodiscard]] int normalsOffset() const { return vertex_count * static_cast<int>(sizeof(float) * 3); }
		[[nodiscard]] int tangentsOffset() const { return normalsOffset() + vertex_count * static_cast<int>(sizeof(float) * 3); }
		[[nodiscard]] int colorsOffset() const { return tangentsOffset() + vertex_count * static_cast<int>(sizeof(float) * 3); }
		[[nodiscard]] int uvsOffset() const { return colorsOffset() + vertex_count * static_cast<int>(sizeof(uint8_t) * 4); }
		[[nodiscard]] int vboSize() const
		{
			return uvsOffset() + vertex_count * static_cast<int>(sizeof(float) * 2);
		}
	};

	/// Run ozz SkinningJob(s) for every mesh part into `out` (CPU only, thread-safe per distinct out).
	bool SkinAnimatedMeshToCache(const AnimatedMesh& mesh, ozz::span<const ozz::math::Float4x4> skinning_matrices, SkinnedMeshFrameCache& out);

	/// ozz multithread-sample style divide-and-conquer over [0, count).
	/// grain: max items processed on one leaf task before splitting with std::async.
	void ParallelForIndex(int count, int grain, const std::function<void(int)>& fn);

} // namespace Engine
