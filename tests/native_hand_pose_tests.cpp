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
using RiddickVR::ComputeNativeHandPlacementAlongAxis;
using RiddickVR::FindNativeHandMesh;
using RiddickVR::MapNativeHandNormal;
using RiddickVR::MapNativeHandPoint;
using RiddickVR::NativeHandMesh;
using RiddickVR::NativeHandPlacement;
using RiddickVR::RotateNativeHandPlacement;

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

    // --- GH #64: hand-local orientation correction composes after the basis ---
    {
        NativeHandPlacement copy = left; // positional copy (no GL, POD struct)
        RotateNativeHandPlacement(copy, 0.0f, 0.0f, 0.0f);
        Check(Near(Det3(copy.rot), 1.0f) && Near(copy.rot[0][0], 1.0f) &&
                  Near(copy.rot[1][1], 1.0f) && Near(copy.rot[2][2], 1.0f),
              "zero angles leave the orientation correction as identity");

        // Yaw +90 about local +Y: local +X -> +Z, and a base-mapped +X offset
        // must equal R * (base result).
        RotateNativeHandPlacement(copy, 90.0f, 0.0f, 0.0f);
        Check(Near(Det3(copy.rot), 1.0f), "yaw correction matrix is orthonormal");
        Check(Near(copy.rot[0][2], 1.0f) && Near(copy.rot[0][0], 0.0f),
              "yaw +90 maps local +X onto +Z");

        // Base result for the +X model offset on the unrotated placement.
        float baseOut[3];
        MapNativeHandPoint(left, plusX, baseOut);
        float rotatedOut[3];
        MapNativeHandPoint(copy, plusX, rotatedOut);
        // R * baseOut, with R = Ry(90).
        const float expX = baseOut[2];
        const float expZ = -baseOut[0];
        Check(Near(rotatedOut[0], expX) && Near(rotatedOut[1], baseOut[1]) &&
                  Near(rotatedOut[2], expZ),
              "rotated point equals R * (basis-mapped point)");

        // Normals rotate identically and stay unit length.
        float nOut[3];
        MapNativeHandNormal(copy, normalIn, nOut); // base local (0,0,1) -> +X
        Check(Near(nOut[0], 1.0f) && Near(nOut[1], 0.0f) && Near(nOut[2], 0.0f),
              "yaw +90 rotates the +Y normal to local +X");
        Check(Near((float)std::sqrt((double)Dot(nOut, nOut)), 1.0f),
              "rotated normal keeps unit length");

        // Pitch +90 about local +X: local +Z -> -Y.
        NativeHandPlacement pitched = left;
        RotateNativeHandPlacement(pitched, 0.0f, 90.0f, 0.0f);
        float pOut[3];
        MapNativeHandNormal(pitched, normalIn, pOut); // base local (0,0,1)
        Check(Near(pOut[0], 0.0f) && Near(pOut[1], -1.0f) && Near(pOut[2], 0.0f),
              "pitch +90 maps local +Z onto -Y");
    }

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

    // --- GH #64: an explicit long axis overrides the largest-extent guess ----
    {
        // Largest extent is Z, but the mesh declares Y as its arm axis: the
        // placement must follow Y (this is exactly the posed right arm's shape).
        const float zmn[3] = { -4.0f, -8.0f, -10.0f };
        const float zmx[3] = { 4.0f, 8.0f, 10.0f };
        NativeHandPlacement heuristic = ComputeNativeHandPlacement(zmn, zmx, false, scale);
        Check(Near(heuristic.basis[2][2], -1.0f), "largest-extent axis follows Z");
        NativeHandPlacement forced = ComputeNativeHandPlacementAlongAxis(zmn, zmx, false, 1, scale);
        Check(forced.valid, "explicit Y axis placement valid");
        Check(Near(forced.basis[2][1], -1.0f), "explicit Y axis overrides the guess");
        CheckOrthonormalRightHanded(forced, "explicit-axis basis orthonormal");
        Check(Near(forced.anchor[1], zmx[1]), "explicit-axis right hand anchors at +Y");
        // Out-of-range / degenerate axes are rejected, never guessed.
        Check(!ComputeNativeHandPlacementAlongAxis(zmn, zmx, false, -1, scale).valid,
              "negative axis rejected");
        Check(!ComputeNativeHandPlacementAlongAxis(zmn, zmx, false, 3, scale).valid,
              "axis > 2 rejected");
        const float flat[3] = { -4.0f, -8.0f, 0.0f };  // zero extent on Y
        Check(!ComputeNativeHandPlacementAlongAxis(zmn, flat, true, 1, scale).valid,
              "zero-extent baked axis rejected");
    }

    // --- GH #64: the baked arm axis makes both hands mirror-symmetric for the
    // bind AND posed AABBs (the posed right AABB would otherwise flip to Z) ---
    {
        const NativeHandMesh* arms[2] = { FindNativeHandMesh("Riddick_LeftArm"),
                                          FindNativeHandMesh("Riddick_RightArm") };
        for (int i = 0; i < 2; ++i) {
            const NativeHandMesh* m = arms[i];
            if (!m) continue;
            const bool isLeft = (i == 0);
            for (int posed = 0; posed < 2; ++posed) {
                if (posed && !m->has_pose()) continue;
                const float* bmn = posed ? m->posed_bounds_min : m->bounds_min;
                const float* bmx = posed ? m->posed_bounds_max : m->bounds_max;
                NativeHandPlacement p = ComputeNativeHandPlacementAlongAxis(
                    bmn, bmx, isLeft, m->arm_axis, scale);
                Check(p.valid, "baked arm axis placement valid");
                if (!p.valid) continue;
                // Mirror: left backward is +Y, right backward is -Y; both up is
                // +Z model, both right (local X) is -X model on the left / +X on
                // the right.
                Check(Near(p.basis[1][2], 1.0f), "arm up is model +Z");
                Check(Near(p.basis[0][0], isLeft ? -1.0f : 1.0f),
                      "arm right mirrors across the body");
                Check(Near(p.basis[2][1], isLeft ? 1.0f : -1.0f),
                      "arm backward is +/-model Y (never flipped to Z)");
                Check(Near(p.basis[2][2], 0.0f), "arm backward has no Z component");
                // Anchor is the hand end along the baked axis.
                const float handEnd = isLeft ? bmn[m->arm_axis] : bmx[m->arm_axis];
                Check(Near(p.anchor[m->arm_axis], handEnd),
                      "baked arm anchors at its hand end");
            }
        }
    }

    if (g_failures != 0) {
        std::fprintf(stderr, "native hand pose: %d failure(s)\n", g_failures);
        return 1;
    }
    std::printf("native hand pose: ok\n");
    return 0;
}
