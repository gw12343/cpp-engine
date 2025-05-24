#include "AnimatedMesh.h"

#include "ozz/base/containers/vector_archive.h"
#include "ozz/base/io/archive.h"

using namespace ozz::io;

// -----------------------------------------------------------------------------
// Engine::Mesh::Part serialization
// -----------------------------------------------------------------------------
void Extern<Engine::Mesh::Part>::Save(OArchive& _archive, const Engine::Mesh::Part* _parts, size_t _count)
{
	for (size_t i = 0; i < _count; ++i) {
		const Engine::Mesh::Part& part = _parts[i];
		_archive << part.positions;
		_archive << part.normals;
		_archive << part.tangents;
		_archive << part.uvs;
		_archive << part.colors;
		_archive << part.joint_indices;
		_archive << part.joint_weights;
	}
}

void Extern<Engine::Mesh::Part>::Load(IArchive& _archive, Engine::Mesh::Part* _parts, size_t _count, uint32_t _version)
{
	(void) _version;
	for (size_t i = 0; i < _count; ++i) {
		Engine::Mesh::Part& part = _parts[i];
		_archive >> part.positions;
		_archive >> part.normals;
		_archive >> part.tangents;
		_archive >> part.uvs;
		_archive >> part.colors;
		_archive >> part.joint_indices;
		_archive >> part.joint_weights;
	}
}

// -----------------------------------------------------------------------------
// Engine::Mesh serialization
// -----------------------------------------------------------------------------
void Extern<Engine::Mesh>::Save(OArchive& _archive, const Engine::Mesh* _meshes, size_t _count)
{
	for (size_t i = 0; i < _count; ++i) {
		const Engine::Mesh& mesh = _meshes[i];
		_archive << mesh.parts;
		_archive << mesh.triangle_indices;
		_archive << mesh.joint_remaps;
		_archive << mesh.inverse_bind_poses;
	}
}

void Extern<Engine::Mesh>::Load(IArchive& _archive, Engine::Mesh* _meshes, size_t _count, uint32_t _version)
{
	(void) _version;
	for (size_t i = 0; i < _count; ++i) {
		Engine::Mesh& mesh = _meshes[i];
		_archive >> mesh.parts;
		_archive >> mesh.triangle_indices;
		_archive >> mesh.joint_remaps;
		_archive >> mesh.inverse_bind_poses;
	}
}