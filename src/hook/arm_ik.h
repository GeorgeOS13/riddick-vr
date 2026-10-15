#pragma once

#include "riddick_vr_protocol.h"
#include <algorithm>
#include <cmath>

namespace RiddickVR::VRArmIK {

inline RiddickVRVector3 Add(const RiddickVRVector3& a, const RiddickVRVector3& b) {
    return { a.x + b.x, a.y + b.y, a.z + b.z };
}

inline RiddickVRVector3 Subtract(const RiddickVRVector3& a, const RiddickVRVector3& b) {
    return { a.x - b.x, a.y - b.y, a.z - b.z };
}

inline RiddickVRVector3 Scale(const RiddickVRVector3& v, float s) {
    return { v.x * s, v.y * s, v.z * s };
}

inline float Length(const RiddickVRVector3& v) {
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

struct TwoBoneSolution {
    RiddickVRVector3 elbow{};
    RiddickVRVector3 wrist{}; // reachable wrist target; may be short of tracked hand
};

// Two-bone elbow solve. The pole is projected onto the plane perpendicular to
// the shoulder/wrist axis, preventing elbow flips. Elbow flex is constrained
// to 8..150 degrees to avoid singular or hyperextended poses.
inline TwoBoneSolution SolveArm(const RiddickVRVector3& shoulder,
        const RiddickVRVector3& wrist, const RiddickVRVector3& pole,
        float upperLength, float lowerLength) {
    RiddickVRVector3 delta = Subtract(wrist, shoulder);
    float distance = Length(delta);
    if (distance < 0.0001f) {
        RiddickVRVector3 fallback = pole;
        float poleLength = Length(fallback);
        return { Add(shoulder, Scale(fallback, upperLength / (poleLength > 0.0001f ? poleLength : 1.0f))), shoulder };
    }

    constexpr float MIN_ELBOW_ANGLE = 0.13962634f;
    constexpr float MAX_ELBOW_ANGLE = 2.61799388f;
    auto reachForAngle = [&](float angle) {
        float d2 = upperLength * upperLength + lowerLength * lowerLength -
                   2.0f * upperLength * lowerLength * cosf(angle);
        return sqrtf((d2 > 0.0001f) ? d2 : 0.0001f);
    };
    float minReach = reachForAngle(MIN_ELBOW_ANGLE);
    float maxReach = reachForAngle(MAX_ELBOW_ANGLE);
    float solveDistance = (std::clamp)(distance, minReach, maxReach);
    RiddickVRVector3 axis = Scale(delta, 1.0f / distance);
    RiddickVRVector3 reachableWrist = Add(shoulder, Scale(axis, solveDistance));

    float poleAlong = pole.x * axis.x + pole.y * axis.y + pole.z * axis.z;
    RiddickVRVector3 bend = Subtract(pole, Scale(axis, poleAlong));
    float bendLength = Length(bend);
    if (bendLength < 0.0001f) {
        RiddickVRVector3 fallback = (fabsf(axis.y) < 0.9f)
            ? RiddickVRVector3{ 0.0f, 1.0f, 0.0f }
            : RiddickVRVector3{ 1.0f, 0.0f, 0.0f };
        poleAlong = fallback.x * axis.x + fallback.y * axis.y + fallback.z * axis.z;
        bend = Subtract(fallback, Scale(axis, poleAlong));
        bendLength = Length(bend);
    }
    bend = Scale(bend, 1.0f / (bendLength > 0.0001f ? bendLength : 1.0f));

    float along = (upperLength * upperLength - lowerLength * lowerLength + solveDistance * solveDistance) /
                  (2.0f * solveDistance);
    float heightSq = upperLength * upperLength - along * along;
    float height = sqrtf((heightSq > 0.0f) ? heightSq : 0.0f);
    RiddickVRVector3 elbow = Add(Add(shoulder, Scale(axis, along)), Scale(bend, height));
    return { elbow, reachableWrist };
}

inline RiddickVRVector3 SolveElbow(const RiddickVRVector3& shoulder,
        const RiddickVRVector3& wrist, const RiddickVRVector3& pole,
        float upperLength, float lowerLength) {
    return SolveArm(shoulder, wrist, pole, upperLength, lowerLength).elbow;
}

inline RiddickVRQuaternion MultiplyQuaternion(const RiddickVRQuaternion& a, const RiddickVRQuaternion& b) {
    return { a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
             a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
             a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
             a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z };
}

inline RiddickVRQuaternion QuaternionFromBasis(const RiddickVRVector3& right,
        const RiddickVRVector3& up, const RiddickVRVector3& forward) {
    float m00 = right.x, m01 = up.x, m02 = forward.x;
    float m10 = right.y, m11 = up.y, m12 = forward.y;
    float m20 = right.z, m21 = up.z, m22 = forward.z;
    RiddickVRQuaternion q{};
    float trace = m00 + m11 + m22;
    if (trace > 0.0f) {
        float s = sqrtf(trace + 1.0f) * 2.0f;
        q.w = 0.25f * s; q.x = (m21 - m12) / s;
        q.y = (m02 - m20) / s; q.z = (m10 - m01) / s;
    } else if (m00 > m11 && m00 > m22) {
        float s = sqrtf(1.0f + m00 - m11 - m22) * 2.0f;
        q.w = (m21 - m12) / s; q.x = 0.25f * s;
        q.y = (m01 + m10) / s; q.z = (m02 + m20) / s;
    } else if (m11 > m22) {
        float s = sqrtf(1.0f + m11 - m00 - m22) * 2.0f;
        q.w = (m02 - m20) / s; q.x = (m01 + m10) / s;
        q.y = 0.25f * s; q.z = (m12 + m21) / s;
    } else {
        float s = sqrtf(1.0f + m22 - m00 - m11) * 2.0f;
        q.w = (m10 - m01) / s; q.x = (m02 + m20) / s;
        q.y = (m12 + m21) / s; q.z = 0.25f * s;
    }
    return q;
}

inline RiddickVRQuaternion ClampWristOrientation(const RiddickVRQuaternion& tracked,
        const RiddickVRVector3& wrist, const RiddickVRVector3& elbow,
        const RiddickVRVector3& pole) {
    RiddickVRVector3 forward = Subtract(elbow, wrist); // local +Z points up the forearm
    float forwardLength = Length(forward);
    if (forwardLength < 0.0001f) return tracked;
    forward = Scale(forward, 1.0f / forwardLength);
    float poleAlong = pole.x * forward.x + pole.y * forward.y + pole.z * forward.z;
    RiddickVRVector3 up = Subtract(pole, Scale(forward, poleAlong));
    float upLength = Length(up);
    if (upLength < 0.0001f) up = { 0.0f, 1.0f, 0.0f };
    else up = Scale(up, 1.0f / upLength);
    RiddickVRVector3 right = { up.y * forward.z - up.z * forward.y,
                               up.z * forward.x - up.x * forward.z,
                               up.x * forward.y - up.y * forward.x };
    float rightLength = Length(right);
    if (rightLength < 0.0001f) return tracked;
    right = Scale(right, 1.0f / rightLength);
    up = { forward.y * right.z - forward.z * right.y,
           forward.z * right.x - forward.x * right.z,
           forward.x * right.y - forward.y * right.x };

    RiddickVRQuaternion base = QuaternionFromBasis(right, up, forward);
    RiddickVRQuaternion relative = MultiplyQuaternion({ -base.x, -base.y, -base.z, base.w }, tracked);
    float roll = atan2f(2.0f * (relative.w * relative.x + relative.y * relative.z),
                        1.0f - 2.0f * (relative.x * relative.x + relative.y * relative.y));
    float sinPitch = 2.0f * (relative.w * relative.y - relative.z * relative.x);
    float pitch = asinf((std::clamp)(sinPitch, -1.0f, 1.0f));
    float yaw = atan2f(2.0f * (relative.w * relative.z + relative.x * relative.y),
                       1.0f - 2.0f * (relative.y * relative.y + relative.z * relative.z));
    constexpr float MAX_FLEX = 0.87266463f; // +/-50 degrees
    constexpr float MAX_DEVIATION = 0.43633231f; // +/-25 degrees
    constexpr float MAX_TWIST = 1.04719755f; // +/-60 degrees
    roll = (std::clamp)(roll, -MAX_FLEX, MAX_FLEX);
    pitch = (std::clamp)(pitch, -MAX_DEVIATION, MAX_DEVIATION);
    yaw = (std::clamp)(yaw, -MAX_TWIST, MAX_TWIST);

    float cr = cosf(roll * 0.5f), sr = sinf(roll * 0.5f);
    float cp = cosf(pitch * 0.5f), sp = sinf(pitch * 0.5f);
    float cy = cosf(yaw * 0.5f), sy = sinf(yaw * 0.5f);
    RiddickVRQuaternion bounded = { sr * cp * cy - cr * sp * sy,
                                    cr * sp * cy + sr * cp * sy,
                                    cr * cp * sy - sr * sp * cy,
                                    cr * cp * cy + sr * sp * sy };
    return MultiplyQuaternion(base, bounded);
}

} // namespace RiddickVR::VRArmIK
