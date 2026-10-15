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

    // Renders both 3D prisoner VR hands for the specified stereoscopic eye pass
    void RenderHands(const RiddickVRSharedState& state, bool isLeftEye, uint32_t eyeWidth, uint32_t eyeHeight);

    // GH #46: per-hand fade factor 0..1 (1 = fully visible, 0 = faded out).
    // Darkening path: output colors are multiplied by the fade factor toward
    // dark (true alpha would need blending state; caller wires logic later).
    void SetHandFade(float leftFade, float rightFade);

private:
    VRHandRenderer() = default;
    ~VRHandRenderer() = default;

    // Renders an individual prisoner hand (left or right)
    // interactionGlow 0..1 = reload/receiver proximity highlight, holdingCartridge draws NanoMED cell, palmHeal draws scanner glow
    // grabGlow 0..1 = object/person interact-volume proximity (vents, terminals, throat-grip corridor)
    void DrawTacticalHand(HandSide side, float grip, float trigger, bool isBoxerGuard, uint32_t buttons,
                           float interactionGlow, bool holdingCartridge, bool palmHeal, float grabGlow = 0.0f);

    // Procedural prisoner hand geometry components (slim Butcher Bay inmate look)
    void DrawForearmGauntlet(HandSide side);
    void DrawPalmAndKnuckleGuard(HandSide side);
    void DrawArticulatedFinger(HandSide side, int fingerIndex, float curl, float splayAngle, float lengthScale);
    void DrawArticulatedThumb(HandSide side, float curl);

    // Geometry utility primitives (colors multiplied by active hand fade)
    void DrawBeveledBox(float x0, float y0, float z0, float x1, float y1, float z1, float r, float g, float b);
    void DrawTaperedCylinder(float z0, float r0, float z1, float r1, int slices, float r, float g, float b);

    float m_leftFade = 1.0f;
    float m_rightFade = 1.0f;
    float m_currentFade = 1.0f;
};

} // namespace RiddickVR
