#pragma once

// GH #46: provisional model-space -> hand-local placement for the baked native
// hand/arm meshes (see native_hand_mesh.h).
//
// Why this exists
// ---------------
// The baked geometry is the engine's raw model space (`.xmd` render space,
// Z-up; ADR-0415). To draw it on a tracked controller it has to be moved into
// the *hand-local* frame the procedural overlay already uses:
//
//     +X = right, +Y = up, +Z = back toward the forearm,
//     fingers point along -Z, units are metres.
//
// Observed vs inferred
// --------------------
// OBSERVED (asset + ADR-0423/0424): each baked mesh carries its model-space
// AABB. The Riddick arm clusters are ~20 units long on model Y, with the hand
// at the -Y end on the left and the +Y end on the right (the left-arm
// fingertips, y ~= -24..-25, are the cluster's minimum-Y vertices; the right
// arm is mirrored).
//
// INFERRED / PROVISIONAL (default OFF, INI-tunable): the arm's long axis is the
// AABB's largest extent; model +Z is hand-local +Y; the *hand end* (the AABB
// extreme on the side selected by `isLeft`) is anchored at the hand-local
// origin; and the caller scales model units to metres. A human with a headset
// must tune `NativeHandScale` / offsets -- nothing here claims a validated fit,
// and the procedural `VRHandRenderer` overlay stays the default path.
//
// This header is dependency-free and header-only so the mapping can be unit
// tested headlessly (tests/native_hand_pose_tests.cpp) without GL or the game.

namespace RiddickVR {

struct NativeHandPlacement {
    bool valid = false;

    // Model-space point placed at the hand-local origin (the AABB "hand end").
    float anchor[3] = { 0.0f, 0.0f, 0.0f };

    // model units -> metres.
    float scale = 1.0f;

    // basis[r] is the model-space unit direction of hand-local axis r
    // (row 0 = local X/right, row 1 = local Y/up, row 2 = local Z/backward).
    float basis[3][3] = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };

    // Model-space AABB the placement was derived from (for diagnostics).
    float aabb_min[3] = { 0.0f, 0.0f, 0.0f };
    float aabb_max[3] = { 0.0f, 0.0f, 0.0f };
};

// Derives the placement for a mesh whose model-space AABB is
// [aabb_min, aabb_max]. `isLeft` selects the side the hand end is on;
// `meters_per_unit` is the world scale applied to the geometry. Returns an
// invalid placement for a degenerate (zero-extent / non-finite) AABB.
inline NativeHandPlacement ComputeNativeHandPlacement(const float aabb_min[3],
                                                      const float aabb_max[3],
                                                      bool isLeft,
                                                      float meters_per_unit) {
    NativeHandPlacement p;
    for (int c = 0; c < 3; ++c) {
        p.aabb_min[c] = aabb_min[c];
        p.aabb_max[c] = aabb_max[c];
    }
    if (!(meters_per_unit > 0.0f)) {
        return p; // invalid: no scale
    }

    float extent[3];
    for (int c = 0; c < 3; ++c) {
        const float lo = aabb_min[c];
        const float hi = aabb_max[c];
        if (!(hi >= lo)) {
            return p; // invalid: inverted / NaN bounds
        }
        extent[c] = hi - lo;
    }

    // Long axis = largest AABB extent (the arm runs along it).
    int long_axis = 0;
    if (extent[1] > extent[long_axis]) long_axis = 1;
    if (extent[2] > extent[long_axis]) long_axis = 2;
    if (!(extent[long_axis] > 0.0f)) {
        return p; // invalid: flat mesh, no axis to orient with
    }

    // Up axis: model +Z unless the arm runs along Z, in which case use +Y.
    const int up_axis = (long_axis == 2) ? 1 : 2;

    // Backward (hand-local +Z) points from the hand toward the shoulder, i.e.
    // away from the hand end. The hand is at the AABB minimum on the left and
    // the maximum on the right (see the header note above).
    const float backward_sign = isLeft ? 1.0f : -1.0f;

    float backward[3] = { 0.0f, 0.0f, 0.0f };
    backward[long_axis] = backward_sign;
    float up[3] = { 0.0f, 0.0f, 0.0f };
    up[up_axis] = 1.0f;

    // Right = up x backward keeps the basis right-handed (local X = Y x Z).
    float right[3] = {
        up[1] * backward[2] - up[2] * backward[1],
        up[2] * backward[0] - up[0] * backward[2],
        up[0] * backward[1] - up[1] * backward[0],
    };

    for (int c = 0; c < 3; ++c) {
        p.basis[0][c] = right[c];
        p.basis[1][c] = up[c];
        p.basis[2][c] = backward[c];
    }

    // Anchor the hand end at the origin; the remaining axes use the AABB
    // mid-point so the arm is centred on the tracked wrist.
    for (int c = 0; c < 3; ++c) {
        p.anchor[c] = 0.5f * (aabb_min[c] + aabb_max[c]);
    }
    p.anchor[long_axis] = (backward_sign > 0.0f) ? aabb_min[long_axis]
                                                 : aabb_max[long_axis];
    p.scale = meters_per_unit;
    p.valid = true;
    return p;
}

// Maps a model-space position into the hand-local frame (metres).
inline void MapNativeHandPoint(const NativeHandPlacement& p, const float in[3],
                               float out[3]) {
    const float d[3] = { in[0] - p.anchor[0], in[1] - p.anchor[1],
                         in[2] - p.anchor[2] };
    for (int r = 0; r < 3; ++r) {
        out[r] = (d[0] * p.basis[r][0] + d[1] * p.basis[r][1] +
                  d[2] * p.basis[r][2]) *
                 p.scale;
    }
}

// Maps a model-space normal into the hand-local frame. The basis is
// orthonormal, so no re-normalisation or scale is applied.
inline void MapNativeHandNormal(const NativeHandPlacement& p, const float in[3],
                                float out[3]) {
    for (int r = 0; r < 3; ++r) {
        out[r] = in[0] * p.basis[r][0] + in[1] * p.basis[r][1] +
                 in[2] * p.basis[r][2];
    }
}

} // namespace RiddickVR
