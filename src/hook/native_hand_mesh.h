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
// context or the game. The geometry is the engine's raw model space (Z-up).
//
// Correction (ADR-0441/0442, wake 35): the stored geometry *is* the skeleton's
// ``NODES`` bind pose — the offline FK check proves the blended mesh equals the
// file geometry to 1.4e-14 over 268 shipped models, so there is no per-cluster
// reference transform and no bind-inverse. The arm clusters merely *look* like a
// separate reference pose because the bind pose itself has the arms held out
// (the Riddick ``NODES`` bind translations spread the arms along +/-Y). ``.xmd``
// still stores no bone rotations (``KEYS`` is a string metadata map, ADR-0421),
// so a consumer that poses the mesh supplies bone animation from an ``.xsa``
// clip and applies it as bind-relative deltas (ADR-0441).
//
// GH #58 (ADR-0443): ``tools/xmd_bake.py`` can additionally bake a **posed**
// variant of each skinned cluster from a shipped first-person ``.xsa`` clip
// (forward kinematics + influence-weighted skinning, evaluated offline). The
// posed arrays are optional (``posed_vertices == nullptr`` when no pose was
// baked, or for unskinned clusters); the renderer still defaults to the bind
// geometry until ``NativeHandPose`` is enabled.

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
    const NativeHandVertex* vertices; // vertex_count entries (bind pose)
    unsigned int vertex_count;
    const unsigned short* indices;    // index_count entries (triangles)
    unsigned int index_count;
    float bounds_min[3];              // model-space AABB (bind pose)
    float bounds_max[3];
    // GH #58: optional pre-posed variant of the same topology, baked offline
    // from a shipped .xsa clip (ADR-0443). nullptr when absent.
    const NativeHandVertex* posed_vertices; // vertex_count entries
    const char* pose_source;                // "<clip>.xsa:<seq>@<t>s" or ""
    float posed_bounds_min[3];
    float posed_bounds_max[3];

    // True when a posed variant is available (GH #58).
    bool has_pose() const { return posed_vertices != nullptr; }
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
