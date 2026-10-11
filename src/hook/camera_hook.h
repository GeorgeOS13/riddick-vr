#pragma once

#include "riddick_vr_protocol.h"
#include <windows.h>

namespace RiddickVR {

class CameraHook {
public:
    static CameraHook& Instance();

    bool Initialize();
    void Update(const RiddickVRSharedState& vrState);

    // Camera decoupling controls
    void SetDecoupleRoll(bool enable) { m_decoupleRoll = enable; }
    void SetDecouplePitch(bool enable) { m_decouplePitch = enable; }

    bool IsCameraLocated() const { return m_cameraMatrixPtr != nullptr; }

private:
    CameraHook() = default;
    ~CameraHook() = default;

    bool ScanForCameraSignature();

    uintptr_t m_moduleBase = 0;
    float* m_cameraMatrixPtr = nullptr;
    float* m_cameraPositionPtr = nullptr;
    float* m_fovPtr = nullptr;

    bool m_decoupleRoll = true;
    bool m_decouplePitch = true;
    bool m_initialized = false;
};

} // namespace RiddickVR
