// mesh.cpp
#include "mesh.h"

#include "ozz/base/containers/vector_archive.h"
#include "ozz/base/io/archive.h"
#include <spdlog/spdlog.h>

using namespace ozz::io;

// -----------------------------------------------------------------------------
// myns::Mesh::Part serialization
// -----------------------------------------------------------------------------
void Extern<myns::Mesh::Part>::Save(OArchive& _archive,
  const myns::Mesh::Part* _parts,
  size_t _count) {
for (size_t i = 0; i < _count; ++i) {
const myns::Mesh::Part& part = _parts[i];
_archive << part.positions;
_archive << part.normals;
_archive << part.tangents;
_archive << part.uvs;
_archive << part.colors;
_archive << part.joint_indices;
_archive << part.joint_weights;
}
}

void Extern<myns::Mesh::Part>::Load(IArchive& _archive,
  myns::Mesh::Part* _parts, size_t _count,
  uint32_t _version) {
(void)_version;
for (size_t i = 0; i < _count; ++i) {
myns::Mesh::Part& part = _parts[i];
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
// myns::Mesh serialization
// -----------------------------------------------------------------------------
void Extern<myns::Mesh>::Save(OArchive& _archive, const myns::Mesh* _meshes,
  size_t _count) {
for (size_t i = 0; i < _count; ++i) {
const myns::Mesh& mesh = _meshes[i];
_archive << mesh.parts;
_archive << mesh.triangle_indices;
_archive << mesh.joint_remaps;
_archive << mesh.inverse_bind_poses;
}
}

void Extern<myns::Mesh>::Load(IArchive& _archive, myns::Mesh* _meshes,
  size_t _count, uint32_t _version) {
(void)_version;
for (size_t i = 0; i < _count; ++i) {
myns::Mesh& mesh = _meshes[i];
_archive >> mesh.parts;
_archive >> mesh.triangle_indices;
_archive >> mesh.joint_remaps;
_archive >> mesh.inverse_bind_poses;
}
  }