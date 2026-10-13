#pragma once

#include "riddick_vr_protocol.h"
#include "ipc_bridge.h"
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <string>
#include <vector>

#define XR_USE_PLATFORM_WIN32
#define XR_USE_GRAPHICS_API_D3D11
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

namespace RiddickVR {

struct OpenXRSwapchain {
    XrSwapchain handle = XR_NULL_HANDLE;
    uint32_t width = 0;
    uint32_t height = 0;
    std::vector<XrSwapchainImageD3D11KHR> images;
};

class OpenXRManager {
public:
    OpenXRManager();
    ~OpenXRManager();

    bool Initialize();
    bool Validate(std::string& outReport);
    void Shutdown();

    void RunLoop(IpcBridgeHost& ipcHost, bool autoExitWithGame = false);
    void RequestExit() { m_running = false; }

    bool IsSessionActive() const { return m_sessionActive; }
    bool IsOpenXRLoaded() const { return m_openxrAvailable; }

    bool TryConnectOpenXR();
    void TeardownSession();

private:
    bool LoadLoader();
    bool CreateInstance();
    bool InitializeD3D11Device();
    bool InitializeSession();
    bool CreateSpaces();
    bool CreateActions();
    bool CreateSwapchains();
    void DestroySwapchains();

    bool PollEvents();
    void UpdateTracking(RiddickVRSharedState& state, XrTime predictedTime);
    void RenderFrame(IpcBridgeHost& ipcHost);

    bool EnsureSharedTextures(uint64_t leftHandle, uint64_t rightHandle);
    void ReleaseSharedTextures();

    std::string GetXrErrorString(XrResult res);

    // OpenXR Loader handle & function pointers
    HMODULE m_hLoader = nullptr;
    bool m_openxrAvailable = false;

    // Core handles
    XrInstance m_instance = XR_NULL_HANDLE;
    XrSystemId m_systemId = XR_NULL_SYSTEM_ID;
    XrSession m_session = XR_NULL_HANDLE;
    XrSessionState m_sessionState = XR_SESSION_STATE_UNKNOWN;
    XrSpace m_appSpace = XR_NULL_HANDLE;
    XrSpace m_viewSpace = XR_NULL_HANDLE;

    // Action set and action handles
    XrActionSet m_actionSet = XR_NULL_HANDLE;
    XrAction m_aimPoseAction = XR_NULL_HANDLE;
    XrAction m_gripPoseAction = XR_NULL_HANDLE;
    XrAction m_locomotionAction = XR_NULL_HANDLE;
    XrAction m_turnAction = XR_NULL_HANDLE;
    XrAction m_triggerAction = XR_NULL_HANDLE;
    XrAction m_gripAction = XR_NULL_HANDLE;
    XrAction m_primaryBtnAction = XR_NULL_HANDLE;
    XrAction m_secondaryBtnAction = XR_NULL_HANDLE;
    XrAction m_recenterBtnAction = XR_NULL_HANDLE;
    XrAction m_hapticAction = XR_NULL_HANDLE;

    // Subaction paths
    XrPath m_leftHandPath = XR_NULL_PATH;
    XrPath m_rightHandPath = XR_NULL_PATH;

    // Action spaces
    XrSpace m_leftGripSpace = XR_NULL_HANDLE;
    XrSpace m_rightGripSpace = XR_NULL_HANDLE;
    XrSpace m_leftAimSpace = XR_NULL_HANDLE;
    XrSpace m_rightAimSpace = XR_NULL_HANDLE;

    // Swapchains for Left [0] and Right [1] eyes
    OpenXRSwapchain m_swapchains[2];

    // D3D11 Compositor Device
    ID3D11Device* m_d3d11Device = nullptr;
    ID3D11DeviceContext* m_d3d11Context = nullptr;

    // Opened shared textures from Hook process
    uint64_t m_currentLeftHandle = 0;
    uint64_t m_currentRightHandle = 0;
    ID3D11Texture2D* m_sharedLeftTex = nullptr;
    ID3D11Texture2D* m_sharedRightTex = nullptr;

    // Headset display configuration
    uint32_t m_recommendedWidth = 2048;
    uint32_t m_recommendedHeight = 2048;
    XrView m_locatedViews[2] = { {XR_TYPE_VIEW}, {XR_TYPE_VIEW} };

    bool m_initialized = false;
    bool m_running = false;
    bool m_sessionActive = false;
    uint32_t m_frameIndex = 0;

    // Function pointers loaded dynamically from openxr_loader.dll
    PFN_xrGetInstanceProcAddr xrGetInstanceProcAddr = nullptr;
    PFN_xrCreateInstance xrCreateInstance = nullptr;
    PFN_xrDestroyInstance xrDestroyInstance = nullptr;
    PFN_xrGetInstanceProperties xrGetInstanceProperties = nullptr;
    PFN_xrGetSystem xrGetSystem = nullptr;
    PFN_xrGetSystemProperties xrGetSystemProperties = nullptr;
    PFN_xrEnumerateViewConfigurations xrEnumerateViewConfigurations = nullptr;
    PFN_xrEnumerateViewConfigurationViews xrEnumerateViewConfigurationViews = nullptr;
    PFN_xrEnumerateSwapchainFormats xrEnumerateSwapchainFormats = nullptr;
    PFN_xrCreateSession xrCreateSession = nullptr;
    PFN_xrDestroySession xrDestroySession = nullptr;
    PFN_xrBeginSession xrBeginSession = nullptr;
    PFN_xrEndSession xrEndSession = nullptr;
    PFN_xrRequestExitSession xrRequestExitSession = nullptr;
    PFN_xrWaitFrame xrWaitFrame = nullptr;
    PFN_xrBeginFrame xrBeginFrame = nullptr;
    PFN_xrEndFrame xrEndFrame = nullptr;
    PFN_xrLocateViews xrLocateViews = nullptr;
    PFN_xrCreateReferenceSpace xrCreateReferenceSpace = nullptr;
    PFN_xrDestroySpace xrDestroySpace = nullptr;
    PFN_xrLocateSpace xrLocateSpace = nullptr;
    PFN_xrCreateActionSet xrCreateActionSet = nullptr;
    PFN_xrDestroyActionSet xrDestroyActionSet = nullptr;
    PFN_xrCreateAction xrCreateAction = nullptr;
    PFN_xrDestroyAction xrDestroyAction = nullptr;
    PFN_xrStringToPath xrStringToPath = nullptr;
    PFN_xrPathToString xrPathToString = nullptr;
    PFN_xrSuggestInteractionProfileBindings xrSuggestInteractionProfileBindings = nullptr;
    PFN_xrAttachSessionActionSets xrAttachSessionActionSets = nullptr;
    PFN_xrCreateActionSpace xrCreateActionSpace = nullptr;
    PFN_xrSyncActions xrSyncActions = nullptr;
    PFN_xrGetActionStateBoolean xrGetActionStateBoolean = nullptr;
    PFN_xrGetActionStateFloat xrGetActionStateFloat = nullptr;
    PFN_xrGetActionStateVector2f xrGetActionStateVector2f = nullptr;
    PFN_xrGetActionStatePose xrGetActionStatePose = nullptr;
    PFN_xrApplyHapticFeedback xrApplyHapticFeedback = nullptr;
    PFN_xrStopHapticFeedback xrStopHapticFeedback = nullptr;
    PFN_xrCreateSwapchain xrCreateSwapchain = nullptr;
    PFN_xrDestroySwapchain xrDestroySwapchain = nullptr;
    PFN_xrEnumerateSwapchainImages xrEnumerateSwapchainImages = nullptr;
    PFN_xrAcquireSwapchainImage xrAcquireSwapchainImage = nullptr;
    PFN_xrWaitSwapchainImage xrWaitSwapchainImage = nullptr;
    PFN_xrReleaseSwapchainImage xrReleaseSwapchainImage = nullptr;
    PFN_xrPollEvent xrPollEvent = nullptr;
    PFN_xrResultToString xrResultToString = nullptr;

    // Extension functions
    PFN_xrGetD3D11GraphicsRequirementsKHR xrGetD3D11GraphicsRequirementsKHR = nullptr;
};

} // namespace RiddickVR
