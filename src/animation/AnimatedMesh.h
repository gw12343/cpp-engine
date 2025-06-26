#pragma once

#include "ozz/base/containers/vector.h"
#include "ozz/base/io/archive.h"
#include "ozz/base/io/archive_traits.h"
#include "ozz/base/maths/math_archive.h"
#include "ozz/base/maths/simd_math.h"
#include "ozz/base/maths/simd_math_archive.h"
#include "ozz/base/maths/vec_float.h"
#include "ozz/base/platform.h"

#undef max
namespace Engine {

	// Defines a mesh with skinning information (joint indices and weights).
	// The mesh is subdivided into parts that group vertices according to their
	// number of influencing joints. Triangle indices are shared across mesh parts.
	struct Mesh {
		// Number of triangle indices for the mesh.
		[[nodiscard]] int triangle_index_count() const { return static_cast<int>(triangle_indices.size()); }

		// Number of vertices for all mesh parts.
		[[nodiscard]] int vertex_count() const
		{
			int count = 0;
			for (const Part& p : parts) {
				count += p.vertex_count();
			}
			return count;
		}

		// Maximum number of joint influences among all parts.
		[[maybe_unused]] [[nodiscard]] int max_influences_count() const
		{
			int max_count = 0;
			for (const Part& p : parts) {
				max_count = std::max(max_count, p.influences_count());
			}
			return max_count;
		}

		// Test if the mesh has skinning information.
		[[nodiscard]] bool skinned() const { return !inverse_bind_poses.empty(); }

		// Number of joints in the skin.
		[[nodiscard]] int num_joints() const { return static_cast<int>(inverse_bind_poses.size()); }

		// Highest joint index used (relies on sorted joint_remaps).
		[[maybe_unused]] [[nodiscard]] int highest_joint_index() const { return joint_remaps.empty() ? 0 : static_cast<int>(joint_remaps.back()); }

		// A submesh grouping vertices with the same # of influences.
		struct Part {
			[[nodiscard]] int vertex_count() const { return static_cast<int>(positions.size()) / kPositionsCpnts; }
			[[nodiscard]] int influences_count() const
			{
				int vc = vertex_count();
				return (vc == 0 ? 0 : static_cast<int>(joint_indices.size()) / vc);
			}

			typedef ozz::vector<float> Positions;
			Positions                  positions;
			enum { kPositionsCpnts = 3 };

			typedef ozz::vector<float> Normals;
			Normals                    normals;
			enum { kNormalsCpnts = 3 };

			typedef ozz::vector<float> Tangents;
			Tangents                   tangents;
			enum { kTangentsCpnts = 4 };

			typedef ozz::vector<float> UVs;
			UVs                        uvs;
			enum { kUVsCpnts = 2 };

			typedef ozz::vector<uint8_t> Colors;
			Colors                       colors;
			enum { kColorsCpnts = 4 };

			typedef ozz::vector<uint16_t> JointIndices;
			JointIndices                  joint_indices; // stride == influences_count

			typedef ozz::vector<float> JointWeights;
			JointWeights               joint_weights; // stride == influences_count
		};

		typedef ozz::vector<Part> Parts;
		Parts                     parts;

		typedef ozz::vector<uint16_t> TriangleIndices;
		TriangleIndices               triangle_indices;

		typedef ozz::vector<uint16_t> JointRemaps;
		JointRemaps                   joint_remaps;

		typedef ozz::vector<ozz::math::Float4x4> InverseBindPoses;
		InverseBindPoses                         inverse_bind_poses;
	};

} // namespace Engine


// -----------------------------------------------------------------------------
// Register with ozz‑animation’s archive system
// -----------------------------------------------------------------------------

namespace ozz::io {

	// Part
	OZZ_IO_TYPE_TAG("ozz-sample-Mesh-Part", Engine::Mesh::Part)
	OZZ_IO_TYPE_VERSION(1, Engine::Mesh::Part)

	template <>
	struct Extern<Engine::Mesh::Part> {
		static void Save(OArchive& _archive, const Engine::Mesh::Part* _parts, size_t _count);
		static void Load(IArchive& _archive, Engine::Mesh::Part* _parts, size_t _count, uint32_t _version);
	};

	// Mesh
	OZZ_IO_TYPE_TAG("ozz-sample-Mesh", Engine::Mesh)
	OZZ_IO_TYPE_VERSION(1, Engine::Mesh)

	template <>
	struct Extern<Engine::Mesh> {
		static void Save(OArchive& _archive, const Engine::Mesh* _meshes, size_t _count);
		static void Load(IArchive& _archive, Engine::Mesh* _meshes, size_t _count, uint32_t _version);
	};

} // namespace ozz::io
