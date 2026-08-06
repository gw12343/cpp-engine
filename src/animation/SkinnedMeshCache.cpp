#include "SkinnedMeshCache.h"

#include "ozz/base/maths/math_ex.h"
#include "ozz/base/memory/allocator.h"
#include "ozz/base/platform.h"
#include "ozz/geometry/runtime/skinning_job.h"

#include <tracy/Tracy.hpp>

#include <algorithm>
#include <atomic>
#include <cstring>
#include <future>
#include <vector>

namespace Engine {
	namespace {
		// Default white color (matches renderer_impl kDefaultColorsArray path)
		constexpr uint8_t kWhite[4] = {255, 255, 255, 255};

		struct PartRange {
			size_t part_index   = 0;
			size_t vertex_begin = 0;
			size_t vertex_count = 0;
		};

		bool SkinPart(const AnimatedMesh&                   mesh,
		              const PartRange&                      range,
		              ozz::span<const ozz::math::Float4x4>  skinning_matrices,
		              uint8_t*                              vbo_base,
		              int                                   vertex_count_total)
		{
			const AnimatedMesh::Part& part = mesh.parts[range.part_index];
			const size_t part_vertex_count = range.vertex_count;
			if (part_vertex_count == 0) {
				return true;
			}

			const size_t positions_stride = sizeof(float) * 3;
			const size_t normals_stride   = sizeof(float) * 3;
			const size_t tangents_stride  = sizeof(float) * 3;
			const size_t colors_stride    = sizeof(uint8_t) * 4;
			const size_t uvs_stride       = sizeof(float) * 2;

			const size_t positions_offset = 0;
			const size_t normals_offset   = static_cast<size_t>(vertex_count_total) * positions_stride;
			const size_t tangents_offset  = normals_offset + static_cast<size_t>(vertex_count_total) * normals_stride;
			const size_t colors_offset    = tangents_offset + static_cast<size_t>(vertex_count_total) * tangents_stride;
			const size_t uvs_offset       = colors_offset + static_cast<size_t>(vertex_count_total) * colors_stride;

			const size_t processed = range.vertex_begin;

			ozz::geometry::SkinningJob skinning_job;
			skinning_job.vertex_count     = static_cast<int>(part_vertex_count);
			const int influences_count    = part.influences_count();
			skinning_job.influences_count = influences_count;
			skinning_job.joint_matrices   = skinning_matrices;

			skinning_job.joint_indices        = ozz::make_span(part.joint_indices);
			skinning_job.joint_indices_stride = sizeof(uint16_t) * influences_count;
			if (influences_count > 1) {
				skinning_job.joint_weights        = ozz::make_span(part.joint_weights);
				skinning_job.joint_weights_stride = sizeof(float) * (influences_count - 1);
			}

			skinning_job.in_positions        = ozz::make_span(part.positions);
			skinning_job.in_positions_stride = sizeof(float) * AnimatedMesh::Part::kPositionsCpnts;

			float* out_pos_begin = reinterpret_cast<float*>(vbo_base + positions_offset + processed * positions_stride);
			float* out_pos_end   = reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(out_pos_begin) + part_vertex_count * positions_stride);
			skinning_job.out_positions        = {out_pos_begin, out_pos_end};
			skinning_job.out_positions_stride = positions_stride;

			float* out_n_begin = reinterpret_cast<float*>(vbo_base + normals_offset + processed * normals_stride);
			float* out_n_end   = reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(out_n_begin) + part_vertex_count * normals_stride);
			if (part.normals.size() / AnimatedMesh::Part::kNormalsCpnts == part_vertex_count) {
				skinning_job.in_normals        = ozz::make_span(part.normals);
				skinning_job.in_normals_stride = sizeof(float) * AnimatedMesh::Part::kNormalsCpnts;
				skinning_job.out_normals        = {out_n_begin, out_n_end};
				skinning_job.out_normals_stride = normals_stride;
			}
			else {
				for (float* n = out_n_begin; n < out_n_end; n = reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(n) + normals_stride)) {
					n[0] = 0.f;
					n[1] = 1.f;
					n[2] = 0.f;
				}
			}

			float* out_t_begin = reinterpret_cast<float*>(vbo_base + tangents_offset + processed * tangents_stride);
			float* out_t_end   = reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(out_t_begin) + part_vertex_count * tangents_stride);
			// Mesh tangents are 4-component (xyz + handiness); skinning job uses 3 for output xyz.
			if (part.tangents.size() / AnimatedMesh::Part::kTangentsCpnts == part_vertex_count) {
				skinning_job.in_tangents        = ozz::make_span(part.tangents);
				skinning_job.in_tangents_stride = sizeof(float) * AnimatedMesh::Part::kTangentsCpnts;
				skinning_job.out_tangents        = {out_t_begin, out_t_end};
				skinning_job.out_tangents_stride = tangents_stride;
			}
			else {
				for (float* t = out_t_begin; t < out_t_end; t = reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(t) + tangents_stride)) {
					t[0] = 1.f;
					t[1] = 0.f;
					t[2] = 0.f;
				}
			}

			{
				ZoneScopedN("SkinningJob::Run");
				if (!skinning_job.Run()) {
					return false;
				}
			}

			// Colors (not skinned)
			uint8_t* color_dst = vbo_base + colors_offset + processed * colors_stride;
			if (part_vertex_count == part.colors.size() / AnimatedMesh::Part::kColorsCpnts) {
				std::memcpy(color_dst, part.colors.data(), part_vertex_count * colors_stride);
			}
			else {
				for (size_t j = 0; j < part_vertex_count; ++j) {
					std::memcpy(color_dst + j * colors_stride, kWhite, 4);
				}
			}

			// UVs (not skinned)
			float* uv_dst = reinterpret_cast<float*>(vbo_base + uvs_offset + processed * uvs_stride);
			if (part_vertex_count == part.uvs.size() / AnimatedMesh::Part::kUVsCpnts) {
				std::memcpy(uv_dst, part.uvs.data(), part_vertex_count * uvs_stride);
			}
			else {
				std::memset(uv_dst, 0, part_vertex_count * uvs_stride);
			}

			return true;
		}
	} // namespace

	void ParallelForIndex(int count, int grain, const std::function<void(int)>& fn)
	{
		if (count <= 0) {
			return;
		}
		grain = std::max(1, grain);

		std::function<void(int, int)> rec = [&](int begin, int n) {
			if (n <= grain) {
				for (int i = 0; i < n; ++i) {
					fn(begin + i);
				}
				return;
			}
			const int half = n / 2;
			auto      fut  = std::async(std::launch::async, [&, begin, half, n]() { rec(begin + half, n - half); });
			rec(begin, half);
			fut.get();
		};
		rec(0, count);
	}

	bool SkinAnimatedMeshToCache(const AnimatedMesh& mesh, ozz::span<const ozz::math::Float4x4> skinning_matrices, SkinnedMeshFrameCache& out)
	{
		ZoneScopedN("SkinAnimatedMeshToCache");

		const int vertex_count = mesh.vertex_count();
		if (vertex_count <= 0) {
			out.valid        = false;
			out.vertex_count = 0;
			out.vbo.clear();
			return false;
		}

		out.vertex_count = vertex_count;
		const int bytes  = out.vboSize();
		out.vbo.resize(static_cast<size_t>(bytes));
		out.valid = false;

		// Build per-part ranges (vertex offsets into packed buffer)
		std::vector<PartRange> ranges;
		ranges.reserve(mesh.parts.size());
		size_t cursor = 0;
		for (size_t p = 0; p < mesh.parts.size(); ++p) {
			const size_t pvc = static_cast<size_t>(mesh.parts[p].vertex_count());
			if (pvc == 0) {
				continue;
			}
			ranges.push_back(PartRange{p, cursor, pvc});
			cursor += pvc;
		}

		if (ranges.empty()) {
			return false;
		}

		// Parallel skin parts (ozz multithread sample: split with std::async).
		// Each part writes to a non-overlapping vertex range → no locks needed.
		std::atomic<bool> ok{true};
		const int         n = static_cast<int>(ranges.size());
		// grain 1: one SkinningJob per async leaf when few parts (typical character ~6 parts)
		ParallelForIndex(n, 1, [&](int i) {
			if (!SkinPart(mesh, ranges[static_cast<size_t>(i)], skinning_matrices, out.vbo.data(), vertex_count)) {
				ok.store(false, std::memory_order_relaxed);
			}
		});

		out.valid = ok.load(std::memory_order_relaxed);
		return out.valid;
	}

} // namespace Engine
