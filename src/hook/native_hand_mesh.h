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
// BIND-POSE PROVENANCE (corrected, GH #64 / ADR-0444). An earlier revision of
// this header claimed the stored geometry *is* the ``NODES`` bind pose on the
// strength of ``tools/xmd_fk.py --check``. That check cannot support the claim:
// at the bind pose every FK world map is the identity, so the influence-weighted
// blend equals ``position * sum(weights)`` by construction and its
// ``pose_drift`` is identically zero for *any* stored geometry. The direct
// measurement (``tools/xmd_pose.py``, ADR-0424) instead shows the two
// ``Riddick_B05`` arm clusters are **not** at the ``NODES`` bind pose: on their
// shared bones their per-bone geometry offsets differ from the body cluster's by
// ~26-27 units (vs <1 unit for legs/body) and their geometry AABBs are disjoint
// from the body -- a raised reference pose. This does not invalidate the
// ADR-0441 recipe: it mirrors the engine (``FUN_1001f340``), which applies the
// same bind-relative world deltas to the *stored* geometry, so the posed bake
// below is the engine's own transform. It only means the phrase "the bind pose"
// must not be used for the file geometry.
//
// GH #58 (ADR-0443): ``tools/xmd_bake.py`` can additionally bake a **posed**
// variant of each skinned cluster from a shipped first-person ``.xsa`` clip
// (forward kinematics + influence-weighted skinning, evaluated offline). The
// posed arrays are optional (``posed_vertices == nullptr`` when no pose was
// baked, or for unskinned clusters); the renderer still defaults to the bind
// geometry until ``NativeHandPose`` is enabled.
//
// GH #64 / ADR-0444: each mesh carries ``arm_axis``, the model-space axis
// (0 = X, 1 = Y, 2 = Z) of the arm/hand's long direction, baked from the mesh's
// *file* geometry so it is stable across poses. The placement no longer
// re-derives this axis from the (deforming) AABB, which flipped for the posed
// right arm and produced a rotated, mis-anchored hand.

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
    int arm_axis;                     // GH #64: model axis (0/1/2) of the arm's
                                      // long direction, baked from file geometry
                                      // (-1 = none; fall back to largest extent)
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
