#include "arm_ik.h"
#include <cmath>
#include <cstdio>

using ::RiddickVRVector3;
using RiddickVR::VRArmIK::Length;
using RiddickVR::VRArmIK::ClampWristOrientation;
using RiddickVR::VRArmIK::SolveArm;
using RiddickVR::VRArmIK::SolveElbow;
using RiddickVR::VRArmIK::Subtract;

static bool Near(float a, float b, float epsilon = 0.001f) {
    return std::fabs(a - b) <= epsilon;
}

static bool SolveReachCase(float reach) {
    constexpr float upper = 0.36f;
    constexpr float lower = 0.34f;
    const RiddickVRVector3 shoulder{ 0.0f, 0.0f, 0.0f };
    const RiddickVRVector3 wrist{ reach, 0.0f, 0.0f };
    const RiddickVRVector3 pole{ 0.0f, 1.0f, 0.0f };
    RiddickVRVector3 elbow = SolveElbow(shoulder, wrist, pole, upper, lower);
    return Near(Length(Subtract(elbow, shoulder)), upper) &&
           Near(Length(Subtract(wrist, elbow)), lower);
}

int main() {
    if (!SolveReachCase(0.11f) || !SolveReachCase(0.35f) || !SolveReachCase(0.64f)) {
        std::fputs("arm IK segment lengths failed within reachable workspace\n", stderr);
        return 1;
    }

    const RiddickVRVector3 shoulder{ 0.0f, 0.0f, 0.0f };
    const RiddickVRVector3 wrist{ 0.45f, 0.0f, 0.0f };
    const RiddickVRVector3 upPole{ 0.0f, 1.0f, 0.0f };
    const RiddickVRVector3 downPole{ 0.0f, -1.0f, 0.0f };
    auto upElbow = SolveElbow(shoulder, wrist, upPole, 0.36f, 0.34f);
    auto downElbow = SolveElbow(shoulder, wrist, downPole, 0.36f, 0.34f);
    if (!(upElbow.y > 0.0f && downElbow.y < 0.0f)) {
        std::fputs("arm IK pole vector did not select a stable bend plane\n", stderr);
        return 1;
    }

    // Identity forearm frame: a 90-degree tracked wrist flex must clamp at
    // the configured +/-50-degree anatomical limit.
    constexpr float halfPi = 1.57079632679f;
    ::RiddickVRQuaternion tracked{ std::sin(halfPi * 0.5f), 0.0f, 0.0f,
                                   std::cos(halfPi * 0.5f) };
    auto wristQuat = ClampWristOrientation(tracked, { 0.0f, 0.0f, 0.0f },
                                           { 0.0f, 0.0f, 1.0f }, upPole);
    float wristFlex = 2.0f * std::atan2(wristQuat.x, wristQuat.w);
    if (!Near(wristFlex, 0.87266463f, 0.002f)) {
        std::fputs("wrist flex exceeded or missed its anatomical limit\n", stderr);
        return 1;
    }

    // Unreachable controller targets clamp the rendered wrist while preserving
    // both segment lengths, instead of stretching the forearm indefinitely.
    auto extended = SolveArm(shoulder, { 2.0f, 0.0f, 0.0f }, upPole, 0.36f, 0.34f);
    if (!(Length(Subtract(extended.wrist, shoulder)) < 2.0f &&
          Near(Length(Subtract(extended.elbow, shoulder)), 0.36f) &&
          Near(Length(Subtract(extended.wrist, extended.elbow)), 0.34f))) {
        std::fputs("arm IK did not clamp unreachable wrist while preserving bone lengths\n", stderr);
        return 1;
    }
    std::puts("arm IK tests passed");
    return 0;
}
