// Headless tests for the native hand placement mapping (GH #46).
//
// native_hand_pose.h turns a mesh's model-space AABB into the hand-local frame
// used by the procedural VRHandRenderer overlay. These tests pin the mapping
// maths on synthetic AABBs and cross-check the baked asset bounds, with no GL
// context, no game install and no input synthesis.

#include "native_hand_pose.h"
#include "native_hand_mesh.h"

#include <cmath>
#include <cstdio>

using RiddickVR::ComputeNativeHandPlacement;
using RiddickVR::FindNativeHandMesh;
using RiddickVR::MapNativeHandNormal;
using RiddickVR::MapNativeHandPoint;
using RiddickVR::NativeHandMesh;
using RiddickVR::NativeHandPlacement;

namespace {

int g_failures = 0;

void Check(bool ok, const char* what) {
    if (!ok) {
        std::fprintf(stderr, "native hand pose: %s\n", what);
        ++g_failures;
    }
}

bool Near(float a, float b, float eps = 1e-4f) {
    return std::fabs(a - b) <= eps;
}

float Dot(const float a[3], const float b[3]) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

float Det3(const float basis[3][3]) {
    return basis[0][0] * (basis[1][1] * basis[2][2] - basis[1][2] * basis[2][1]) -
           basis[0][1] * (basis[1][0] * basis[2][2] - basis[1][2] * basis[2][0]) +
           basis[0][2] * (basis[1][0] * basis[2][1] - basis[1][1] * basis[2][0]);
}

void CheckOrthonormalRightHanded(const NativeHandPlacement& p, const char* label) {
    for (int r = 0; r < 3; ++r) {
        float len = (float)std::sqrt((double)Dot(p.basis[r], p.basis[r]));
        if (!Near(len, 1.0f)) {
            std::fprintf(stderr, "native hand pose: %s basis row %d not unit (%.4f)\n",
                         label, r, len);
            ++g_failures;
        }
    }
    Check(Near(Det3(p.basis), 1.0f), label);
}

} // namespace

int main() {
    // --- Synthetic arm-like AABB: longest on Y, hand at min-Y (left) -------
    const float mn[3] = { -4.0f, -25.0f, 45.0f };
    const float mx[3] = { 3.0f, -5.0f, 52.0f };
    const float scale = 0.033f;

    NativeHandPlacement left = ComputeNativeHandPlacement(mn, mx, true, scale);
    Check(left.valid, "left placement valid");
    CheckOrthonormalRightHanded(left, "left basis orthonormal/right-handed");

    // Long axis is Y -> +Z (backward) is +Y on the left; +Y (up) is +Z model.
    Check(Near(left.basis[2][0], 0.0f) && Near(left.basis[2][1], 1.0f) &&
              Near(left.basis[2][2], 0.0f),
          "left backward is +Y");
    Check(Near(left.basis[1][0], 0.0f) && Near(left.basis[1][1], 0.0f) &&
              Near(left.basis[1][2], 1.0f),
          "left up is +Z");
    Check(Near(left.basis[0][0], -1.0f), "left right is -X model");

    // Hand end (min-Y, on the AABB centre line) anchors exactly at the origin.
    const float handEnd[3] = { -0.5f, mn[1], 48.5f };
    float out[3];
    MapNativeHandPoint(left, handEnd, out);
    Check(Near(out[0], 0.0f) && Near(out[1], 0.0f) && Near(out[2], 0.0f),
          "left hand end maps to origin");

    // Shoulder end (max-Y) lands straight behind (+Z) at the scaled span.
    const float shoulder[3] = { -0.5f, mx[1], 48.5f };
    MapNativeHandPoint(left, shoulder, out);
    Check(Near(out[0], 0.0f) && Near(out[1], 0.0f), "left shoulder stays on the Z axis");
    Check(Near(out[2], (mx[1] - mn[1]) * scale), "left shoulder span uses the scale");
    Check(out[2] > 0.0f, "left shoulder is behind the hand (+Z)");

    // Off-axis model X maps to local -X (right-handed mirror), scaled.
    const float plusX[3] = { -0.5f + 1.0f, mn[1], 48.5f };
    MapNativeHandPoint(left, plusX, out);
    Check(Near(out[0], -1.0f * scale), "left +X model maps to -X local * scale");

    // --- Right side mirrors the lateral + longitudinal directions -----------
    NativeHandPlacement right = ComputeNativeHandPlacement(mn, mx, false, scale);
    Check(right.valid, "right placement valid");
    CheckOrthonormalRightHanded(right, "right basis orthonormal/right-handed");
    Check(Near(right.basis[2][1], -1.0f), "right backward is -Y");
    Check(Near(right.basis[0][0], 1.0f), "right right is +X model");
    // Right hand end is the max-Y extreme -> shoulder maps behind (+Z).
    const float rightHandEnd[3] = { -0.5f, mx[1], 48.5f };
    MapNativeHandPoint(right, rightHandEnd, out);
    Check(Near(out[0], 0.0f) && Near(out[1], 0.0f) && Near(out[2], 0.0f),
          "right hand end maps to origin");
    const float rightShoulder[3] = { -0.5f, mn[1], 48.5f };
    MapNativeHandPoint(right, rightShoulder, out);
    Check(out[2] > 0.0f && Near(out[2], (mx[1] - mn[1]) * scale),
          "right shoulder is behind the hand at the scaled span");

    // --- Normals: unit directions map through the basis, length preserved ---
    const float normalIn[3] = { 0.0f, 1.0f, 0.0f };
    MapNativeHandNormal(left, normalIn, out);
    Check(Near(out[0], 0.0f) && Near(out[1], 0.0f) && Near(out[2], 1.0f),
          "left +Y normal maps to +Z local");
    MapNativeHandNormal(right, normalIn, out);
    Check(Near(out[2], -1.0f), "right +Y normal maps to -Z local");

    // --- Degenerate / invalid inputs are rejected, never silently mapped ----
    const float zeroX[3] = { 0, 0, 0 };
    Check(!ComputeNativeHandPlacement(zeroX, zeroX, true, scale).valid,
          "zero-extent AABB invalid");
    const float inverted[3] = { -4.0f, 0.0f, 0.0f };
    Check(!ComputeNativeHandPlacement(mn, inverted, true, scale).valid,
          "inverted bounds invalid");
    Check(!ComputeNativeHandPlacement(mn, mx, true, 0.0f).valid,
          "non-positive scale invalid");
    Check(!ComputeNativeHandPlacement(mn, mx, true, -1.0f).valid,
          "negative scale invalid");

    // --- Baked asset cross-check: bounds give a valid, Y-long placement -----
    const NativeHandMesh* leftArm = FindNativeHandMesh("Riddick_LeftArm");
    const NativeHandMesh* rightArm = FindNativeHandMesh("Riddick_RightArm");
    Check(leftArm != nullptr && rightArm != nullptr, "baked arm meshes present");
    if (leftArm && rightArm) {
        const NativeHandMesh* arms[2] = { leftArm, rightArm };
        for (int i = 0; i < 2; ++i) {
            const NativeHandMesh* m = arms[i];
            const float ex = m->bounds_max[0] - m->bounds_min[0];
            const float ey = m->bounds_max[1] - m->bounds_min[1];
            const float ez = m->bounds_max[2] - m->bounds_min[2];
            Check(ey > ex && ey > ez, "arm mesh is longest on model Y");
        }
        NativeHandPlacement lp = ComputeNativeHandPlacement(
            leftArm->bounds_min, leftArm->bounds_max, true, 0.033f);
        Check(lp.valid, "baked left arm placement valid");
        Check(Near(lp.anchor[1], leftArm->bounds_min[1]),
              "baked left arm anchors at the min-Y hand end");
        NativeHandPlacement rp = ComputeNativeHandPlacement(
            rightArm->bounds_min, rightArm->bounds_max, false, 0.033f);
        Check(rp.valid, "baked right arm placement valid");
        Check(Near(rp.anchor[1], rightArm->bounds_max[1]),
              "baked right arm anchors at the max-Y hand end");
    }

    if (g_failures != 0) {
        std::fprintf(stderr, "native hand pose: %d failure(s)\n", g_failures);
        return 1;
    }
    std::printf("native hand pose: ok\n");
    return 0;
}
