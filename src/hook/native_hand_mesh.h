#pragma once

// GH #55 -> GH #46: native first-person hand/arm asset.
//
// The real Riddick arm/hand geometry is baked out of the shipped
// ``MOS DATAFILE2.0`` ``.xmd`` models by ``tools/xmd_bake.py`` into
// ``native_hand_mesh_data.h`` (generated; do not edit) and compiled straight
// into the hook, so the game process never has to parse the binary format or
// touch the game install at runtime.
//
// This header is intentionally dependency-free and header-only so it can be
// unit-tested headlessly (``tests/native_hand_mesh_tests.cpp``) without a GL
// context or the game. The geometry is the engine's raw model space (Z-up) and
// is the artist's bind pose: ``.xmd`` stores bind-pose translations only, so a
// consumer that poses the mesh must animate the bones itself. Until that
// pose pipeline lands the procedural ``VRHandRenderer`` stays the default and
// this asset is consumed read-only.

namespace RiddickVR {

struct NativeHandVertex {
    float position[3];
    float normal[3];
    float uv[2];
};

struct NativeHandMesh {
    const char* name;                 // stable asset name, e.g. "Riddick_LeftArm"
    const char* source;               // .xmd path relative to Content/Models
    int cluster;                      // source cluster index within that model
    int surface_index;                // CLUSTER/MISCELLANEOUS surface id
    const NativeHandVertex* vertices; // vertex_count entries
    unsigned int vertex_count;
    const unsigned short* indices;    // index_count entries (triangles)
    unsigned int index_count;
    float bounds_min[3];              // model-space AABB
    float bounds_max[3];
};

} // namespace RiddickVR

#include "native_hand_mesh_data.h"

#include <cstring>

namespace RiddickVR {

inline int NativeHandMeshCount() {
    return NativeHandMeshData::kMeshCount;
}

// Clamped accessor: never returns a dangling reference.
inline const NativeHandMesh& NativeHandMeshAt(int index) {
    if (index < 0 || index >= NativeHandMeshData::kMeshCount) {
        index = 0;
    }
    return NativeHandMeshData::kMeshes[index];
}

inline const NativeHandMesh* FindNativeHandMesh(const char* name) {
    if (!name) {
        return nullptr;
    }
    for (int i = 0; i < NativeHandMeshData::kMeshCount; ++i) {
        if (std::strcmp(NativeHandMeshData::kMeshes[i].name, name) == 0) {
            return &NativeHandMeshData::kMeshes[i];
        }
    }
    return nullptr;
}

} // namespace RiddickVR
