#pragma once

#include "riddick_vr_protocol.h"
#include <windows.h>
#include <GL/gl.h>

namespace RiddickVR {

enum class HandSide {
    Left,
    Right
};

class VRHandRenderer {
public:
    static VRHandRenderer& Instance();

    // Renders both 3D tactical VR hands for the specified stereoscopic eye pass
    void RenderHands(const RiddickVRSharedState& state, bool isLeftEye, uint32_t eyeWidth, uint32_t eyeHeight);

private:
    VRHandRenderer() = default;
    ~VRHandRenderer() = default;

    // Renders an individual tactical glove (left or right)
    void DrawTacticalHand(HandSide side, float grip, float trigger, bool isBoxerGuard, uint32_t buttons);

    // Procedural tactical glove geometry components
    void DrawForearmGauntlet(HandSide side);
    void DrawPalmAndKnuckleGuard(HandSide side);
    void DrawArticulatedFinger(HandSide side, int fingerIndex, float curl, float splayAngle, float lengthScale);
    void DrawArticulatedThumb(HandSide side, float curl);

    // Geometry utility primitives
    void DrawBeveledBox(float x0, float y0, float z0, float x1, float y1, float z1, float r, float g, float b);
    void DrawTaperedCylinder(float z0, float r0, float z1, float r1, int slices, float r, float g, float b);
};

} // namespace RiddickVR
