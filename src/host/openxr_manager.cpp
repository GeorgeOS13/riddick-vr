#include "openxr_manager.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <cmath>

namespace RiddickVR {

static RiddickVRMatrix4x4 CreateProjectionFromFov(const XrFovf& fov, float nearZ = 0.05f, float farZ = 1000.0f) {
    RiddickVRMatrix4x4 result{};
    for (int i = 0; i < 16; ++i) result.m[i] = 0.0f;

    const float tanLeft = tanf(fov.angleLeft);
    const float tanRight = tanf(fov.angleRight);
    const float tanDown = tanf(fov.angleDown);
    const float tanUp = tanf(fov.angleUp);

    const float idx = 1.0f / (tanRight - tanLeft);
    const float idy = 1.0f / (tanUp - tanDown);
    const float idz = 1.0f / (farZ - nearZ);

    result.m[0] = 2.0f * idx;
    result.m[5] = 2.0f * idy;
    result.m[8] = (tanRight + tanLeft) * idx;
    result.m[9] = (tanUp + tanDown) * idy;
    result.m[10] = -(farZ + nearZ) * idz;
    result.m[11] = -1.0f;
    result.m[14] = -(2.0f * farZ * nearZ) * idz;
    result.m[15] = 0.0f;

    return result;
}

static RiddickVRMatrix4x4 CreateViewFromPose(const XrPosef& pose) {
    RiddickVRMatrix4x4 result{};
    const float qx = pose.orientation.x;
    const float qy = pose.orientation.y;
    const float qz = pose.orientation.z;
    const float qw = pose.orientation.w;

    // Rotation matrix from quaternion (R)
    const float r00 = 1.0f - 2.0f * (qy * qy + qz * qz);
    const float r01 = 2.0f * (qx * qy - qz * qw);
    const float r02 = 2.0f * (qx * qz + qy * qw);

    const float r10 = 2.0f * (qx * qy + qz * qw);
    const float r11 = 1.0f - 2.0f * (qx * qx + qz * qz);
    const float r12 = 2.0f * (qy * qz - qx * qw);

    const float r20 = 2.0f * (qx * qz - qy * qw);
    const float r21 = 2.0f * (qy * qz + qx * qw);
    const float r22 = 1.0f - 2.0f * (qx * qx + qy * qy);

    const float px = pose.position.x;
    const float py = pose.position.y;
    const float pz = pose.position.z;

    // Invert rigid transform: R^T and -R^T * p
    result.m[0] = r00;
    result.m[1] = r01;
    result.m[2] = r02;
    result.m[3] = 0.0f;

    result.m[4] = r10;
    result.m[5] = r11;
    result.m[6] = r12;
    result.m[7] = 0.0f;

    result.m[8] = r20;
    result.m[9] = r21;
    result.m[10] = r22;
    result.m[11] = 0.0f;

    result.m[12] = -(r00 * px + r10 * py + r20 * pz);
    result.m[13] = -(r01 * px + r11 * py + r21 * pz);
    result.m[14] = -(r02 * px + r12 * py + r22 * pz);
    result.m[15] = 1.0f;

    return result;
}

OpenXRManager::OpenXRManager() {
    m_locatedViews[0].pose.orientation.w = 1.0f;
    m_locatedViews[0].fov = { -0.785f, 0.785f, 0.785f, -0.785f };
    m_locatedViews[1].pose.orientation.w = 1.0f;
    m_locatedViews[1].fov = { -0.785f, 0.785f, 0.785f, -0.785f };
}

OpenXRManager::~OpenXRManager() {
    Shutdown();
}

std::string OpenXRManager::GetXrErrorString(XrResult res) {
    if (xrResultToString && m_instance != XR_NULL_HANDLE) {
        char buffer[XR_MAX_RESULT_STRING_SIZE] = {0};
        if (XR_SUCCEEDED(xrResultToString(m_instance, res, buffer))) {
            return std::string(buffer);
        }
    }
    return "XrResult " + std::to_string(res);
}

bool OpenXRManager::LoadLoader() {
    if (m_hLoader) return true;

    // Search order: adjacent directory, current working dir, system path
    m_hLoader = LoadLibraryA("openxr_loader.dll");
    if (!m_hLoader) {
        m_hLoader = LoadLibraryA("dist\\openxr_loader.dll");
    }

    if (!m_hLoader) {
        return false;
    }

    // Resolve xrGetInstanceProcAddr
    xrGetInstanceProcAddr = (PFN_xrGetInstanceProcAddr)GetProcAddress(m_hLoader, "xrGetInstanceProcAddr");
    if (!xrGetInstanceProcAddr) {
        FreeLibrary(m_hLoader);
        m_hLoader = nullptr;
        return false;
    }

    // Resolve exported core functions
#define LOAD_PROC(name) name = (PFN_##name)GetProcAddress(m_hLoader, #name)
    LOAD_PROC(xrCreateInstance);
    LOAD_PROC(xrDestroyInstance);
    LOAD_PROC(xrGetInstanceProperties);
    LOAD_PROC(xrGetSystem);
    LOAD_PROC(xrGetSystemProperties);
    LOAD_PROC(xrEnumerateViewConfigurations);
    LOAD_PROC(xrEnumerateViewConfigurationViews);
    LOAD_PROC(xrEnumerateSwapchainFormats);
    LOAD_PROC(xrCreateSession);
    LOAD_PROC(xrDestroySession);
    LOAD_PROC(xrBeginSession);
    LOAD_PROC(xrEndSession);
    LOAD_PROC(xrRequestExitSession);
    LOAD_PROC(xrWaitFrame);
    LOAD_PROC(xrBeginFrame);
    LOAD_PROC(xrEndFrame);
    LOAD_PROC(xrLocateViews);
    LOAD_PROC(xrCreateReferenceSpace);
    LOAD_PROC(xrDestroySpace);
    LOAD_PROC(xrLocateSpace);
    LOAD_PROC(xrCreateActionSet);
    LOAD_PROC(xrDestroyActionSet);
    LOAD_PROC(xrCreateAction);
    LOAD_PROC(xrDestroyAction);
    LOAD_PROC(xrStringToPath);
    LOAD_PROC(xrPathToString);
    LOAD_PROC(xrSuggestInteractionProfileBindings);
    LOAD_PROC(xrAttachSessionActionSets);
    LOAD_PROC(xrCreateActionSpace);
    LOAD_PROC(xrSyncActions);
    LOAD_PROC(xrGetActionStateBoolean);
    LOAD_PROC(xrGetActionStateFloat);
    LOAD_PROC(xrGetActionStateVector2f);
    LOAD_PROC(xrGetActionStatePose);
    LOAD_PROC(xrApplyHapticFeedback);
    LOAD_PROC(xrStopHapticFeedback);
    LOAD_PROC(xrCreateSwapchain);
    LOAD_PROC(xrDestroySwapchain);
    LOAD_PROC(xrEnumerateSwapchainImages);
    LOAD_PROC(xrAcquireSwapchainImage);
    LOAD_PROC(xrWaitSwapchainImage);
    LOAD_PROC(xrReleaseSwapchainImage);
    LOAD_PROC(xrPollEvent);
    LOAD_PROC(xrResultToString);
#undef LOAD_PROC

    m_openxrAvailable = (xrCreateInstance != nullptr);
    return m_openxrAvailable;
}

bool OpenXRManager::CreateInstance() {
    if (!m_openxrAvailable || !xrCreateInstance) return false;

    const char* enabledExtensions[] = {
        XR_KHR_D3D11_ENABLE_EXTENSION_NAME
    };

    XrInstanceCreateInfo createInfo{XR_TYPE_INSTANCE_CREATE_INFO};
    strcpy_s(createInfo.applicationInfo.applicationName, "Riddick VR");
    createInfo.applicationInfo.applicationVersion = 1;
    strcpy_s(createInfo.applicationInfo.engineName, "Starbreeze Engine");
    createInfo.applicationInfo.engineVersion = 1;
    createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
    createInfo.enabledExtensionCount = 1;
    createInfo.enabledExtensionNames = enabledExtensions;

    XrResult res = xrCreateInstance(&createInfo, &m_instance);
    if (XR_FAILED(res) || m_instance == XR_NULL_HANDLE) {
        static DWORD s_lastLogTime = 0;
        DWORD now = GetTickCount();
        if (now - s_lastLogTime >= 5000) {
            std::cout << "[Host] Searching for active OpenXR headset runtime (" << GetXrErrorString(res) << ")... Ensure Virtual Desktop is streaming." << std::endl;
            s_lastLogTime = now;
        }
        return false;
    }

    // Load D3D11 extension function
    xrGetInstanceProcAddr(m_instance, "xrGetD3D11GraphicsRequirementsKHR", (PFN_xrVoidFunction*)&xrGetD3D11GraphicsRequirementsKHR);
    return true;
}

bool OpenXRManager::InitializeD3D11Device() {
    if (m_d3d11Device) return true;

    D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    D3D_FEATURE_LEVEL featureLevel;
    HRESULT hr = D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,
        featureLevels,
        2,
        D3D11_SDK_VERSION,
        &m_d3d11Device,
        &featureLevel,
        &m_d3d11Context
    );

    return SUCCEEDED(hr) && (m_d3d11Device != nullptr);
}

bool OpenXRManager::InitializeSession() {
    if (!m_instance) return false;

    // Get System ID for HMD
    XrSystemGetInfo systemGetInfo{XR_TYPE_SYSTEM_GET_INFO};
    systemGetInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    XrResult res = xrGetSystem(m_instance, &systemGetInfo, &m_systemId);
    if (XR_FAILED(res)) {
        std::cout << "[Host] Notice: No VR headset detected (" << GetXrErrorString(res) << ")." << std::endl;
        return false;
    }

    // Query D3D11 Requirements
    if (xrGetD3D11GraphicsRequirementsKHR) {
        XrGraphicsRequirementsD3D11KHR graphicsRequirements{XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR};
        xrGetD3D11GraphicsRequirementsKHR(m_instance, m_systemId, &graphicsRequirements);
    }

    if (!InitializeD3D11Device()) {
        std::cerr << "[Host] ERROR: Failed to create Direct3D 11 device for OpenXR session!" << std::endl;
        return false;
    }

    // Query recommended view configuration
    uint32_t viewConfigCount = 0;
    xrEnumerateViewConfigurationViews(m_instance, m_systemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 0, &viewConfigCount, nullptr);
    if (viewConfigCount >= 2) {
        std::vector<XrViewConfigurationView> views(viewConfigCount, {XR_TYPE_VIEW_CONFIGURATION_VIEW});
        xrEnumerateViewConfigurationViews(m_instance, m_systemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, viewConfigCount, &viewConfigCount, views.data());
        m_recommendedWidth = views[0].recommendedImageRectWidth;
        m_recommendedHeight = views[0].recommendedImageRectHeight;
        std::cout << "[Host] Headset Recommended Eye Resolution: " << m_recommendedWidth << "x" << m_recommendedHeight << std::endl;
    }

    // Create Session
    XrGraphicsBindingD3D11KHR graphicsBinding{XR_TYPE_GRAPHICS_BINDING_D3D11_KHR};
    graphicsBinding.device = m_d3d11Device;

    XrSessionCreateInfo sessionCreateInfo{XR_TYPE_SESSION_CREATE_INFO};
    sessionCreateInfo.next = &graphicsBinding;
    sessionCreateInfo.systemId = m_systemId;

    res = xrCreateSession(m_instance, &sessionCreateInfo, &m_session);
    if (XR_FAILED(res)) {
        std::cerr << "[Host] ERROR: xrCreateSession failed (" << GetXrErrorString(res) << ")" << std::endl;
        return false;
    }

    CreateSpaces();
    CreateActions();
    CreateSwapchains();

    // Begin Session (if runtime is already in READY state, or wait for event)
    XrSessionBeginInfo beginInfo{XR_TYPE_SESSION_BEGIN_INFO};
    beginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
    res = xrBeginSession(m_session, &beginInfo);
    if (XR_SUCCEEDED(res)) {
        m_sessionActive = true;
        std::cout << "[Host] OpenXR Session ACTIVE and running!" << std::endl;
    } else if (res == XR_ERROR_SESSION_NOT_READY) {
        std::cout << "[Host] OpenXR Session initialized; awaiting runtime READY state..." << std::endl;
    } else {
        std::cout << "[Host] Notice: xrBeginSession returned " << GetXrErrorString(res) << std::endl;
    }

    return (m_session != XR_NULL_HANDLE);
}

bool OpenXRManager::CreateSpaces() {
    if (!m_session) return false;

    // Try Stage space first for room-scale, fallback to Local
    XrReferenceSpaceCreateInfo spaceCreateInfo{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
    spaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
    spaceCreateInfo.poseInReferenceSpace.orientation.w = 1.0f;

    XrResult res = xrCreateReferenceSpace(m_session, &spaceCreateInfo, &m_appSpace);
    if (XR_FAILED(res)) {
        spaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
        xrCreateReferenceSpace(m_session, &spaceCreateInfo, &m_appSpace);
    }

    // View space (HMD gaze)
    spaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
    xrCreateReferenceSpace(m_session, &spaceCreateInfo, &m_viewSpace);

    return (m_appSpace != XR_NULL_HANDLE);
}

bool OpenXRManager::CreateActions() {
    if (!m_session || !m_instance) return false;

    // Action set: gameplay
    XrActionSetCreateInfo actionSetInfo{XR_TYPE_ACTION_SET_CREATE_INFO};
    strcpy_s(actionSetInfo.actionSetName, "gameplay");
    strcpy_s(actionSetInfo.localizedActionSetName, "Gameplay");
    actionSetInfo.priority = 0;
    if (XR_FAILED(xrCreateActionSet(m_instance, &actionSetInfo, &m_actionSet))) return false;

    // Subaction paths
    xrStringToPath(m_instance, "/user/hand/left", &m_leftHandPath);
    xrStringToPath(m_instance, "/user/hand/right", &m_rightHandPath);
    XrPath handSubactionPaths[] = { m_leftHandPath, m_rightHandPath };

    auto CreateAction = [&](const char* name, const char* localized, XrActionType type, uint32_t count, const XrPath* subactions, XrAction* outAction) {
        XrActionCreateInfo info{XR_TYPE_ACTION_CREATE_INFO};
        strcpy_s(info.actionName, name);
        strcpy_s(info.localizedActionName, localized);
        info.actionType = type;
        info.countSubactionPaths = count;
        info.subactionPaths = subactions;
        return XR_SUCCEEDED(xrCreateAction(m_actionSet, &info, outAction));
    };

    CreateAction("aim_pose", "Aim Pose", XR_ACTION_TYPE_POSE_INPUT, 2, handSubactionPaths, &m_aimPoseAction);
    CreateAction("grip_pose", "Grip Pose", XR_ACTION_TYPE_POSE_INPUT, 2, handSubactionPaths, &m_gripPoseAction);
    CreateAction("locomotion", "Locomotion", XR_ACTION_TYPE_VECTOR2F_INPUT, 2, handSubactionPaths, &m_locomotionAction);
    CreateAction("turn", "Turn", XR_ACTION_TYPE_VECTOR2F_INPUT, 2, handSubactionPaths, &m_turnAction);
    CreateAction("trigger", "Trigger", XR_ACTION_TYPE_FLOAT_INPUT, 2, handSubactionPaths, &m_triggerAction);
    CreateAction("grip", "Grip", XR_ACTION_TYPE_FLOAT_INPUT, 2, handSubactionPaths, &m_gripAction);
    CreateAction("primary_btn", "Primary Button", XR_ACTION_TYPE_BOOLEAN_INPUT, 2, handSubactionPaths, &m_primaryBtnAction);
    CreateAction("secondary_btn", "Secondary Button", XR_ACTION_TYPE_BOOLEAN_INPUT, 2, handSubactionPaths, &m_secondaryBtnAction);
    CreateAction("recenter_btn", "Recenter Button", XR_ACTION_TYPE_BOOLEAN_INPUT, 2, handSubactionPaths, &m_recenterBtnAction);
    CreateAction("haptic", "Haptic Vibration", XR_ACTION_TYPE_VIBRATION_OUTPUT, 2, handSubactionPaths, &m_hapticAction);

    // Suggest bindings for Touch controller
    XrPath touchProfile;
    xrStringToPath(m_instance, "/interaction_profiles/oculus/touch_controller", &touchProfile);

    auto Path = [&](const char* str) -> XrPath {
        XrPath p;
        xrStringToPath(m_instance, str, &p);
        return p;
    };

    std::vector<XrActionSuggestedBinding> touchBindings = {
        { m_aimPoseAction, Path("/user/hand/left/input/aim/pose") },
        { m_aimPoseAction, Path("/user/hand/right/input/aim/pose") },
        { m_gripPoseAction, Path("/user/hand/left/input/grip/pose") },
        { m_gripPoseAction, Path("/user/hand/right/input/grip/pose") },
        { m_locomotionAction, Path("/user/hand/left/input/thumbstick") },
        { m_turnAction, Path("/user/hand/right/input/thumbstick") },
        { m_triggerAction, Path("/user/hand/left/input/trigger/value") },
        { m_triggerAction, Path("/user/hand/right/input/trigger/value") },
        { m_gripAction, Path("/user/hand/left/input/squeeze/value") },
        { m_gripAction, Path("/user/hand/right/input/squeeze/value") },
        { m_primaryBtnAction, Path("/user/hand/left/input/x/click") },
        { m_primaryBtnAction, Path("/user/hand/right/input/a/click") },
        { m_secondaryBtnAction, Path("/user/hand/left/input/y/click") },
        { m_secondaryBtnAction, Path("/user/hand/right/input/b/click") },
        { m_recenterBtnAction, Path("/user/hand/right/input/thumbstick/click") },
        { m_hapticAction, Path("/user/hand/left/output/haptic") },
        { m_hapticAction, Path("/user/hand/right/output/haptic") }
    };

    XrInteractionProfileSuggestedBinding suggestedBindings{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
    suggestedBindings.interactionProfile = touchProfile;
    suggestedBindings.suggestedBindings = touchBindings.data();
    suggestedBindings.countSuggestedBindings = (uint32_t)touchBindings.size();
    xrSuggestInteractionProfileBindings(m_instance, &suggestedBindings);

    // Attach to session
    XrSessionActionSetsAttachInfo attachInfo{XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO};
    attachInfo.countActionSets = 1;
    attachInfo.actionSets = &m_actionSet;
    xrAttachSessionActionSets(m_session, &attachInfo);

    // Create action spaces for grip and aim
    auto CreateActionSpace = [&](XrAction action, XrPath subaction, XrSpace* outSpace) {
        XrActionSpaceCreateInfo spaceInfo{XR_TYPE_ACTION_SPACE_CREATE_INFO};
        spaceInfo.action = action;
        spaceInfo.subactionPath = subaction;
        spaceInfo.poseInActionSpace.orientation.w = 1.0f;
        xrCreateActionSpace(m_session, &spaceInfo, outSpace);
    };

    CreateActionSpace(m_gripPoseAction, m_leftHandPath, &m_leftGripSpace);
    CreateActionSpace(m_gripPoseAction, m_rightHandPath, &m_rightGripSpace);
    CreateActionSpace(m_aimPoseAction, m_leftHandPath, &m_leftAimSpace);
    CreateActionSpace(m_aimPoseAction, m_rightHandPath, &m_rightAimSpace);

    return true;
}

bool OpenXRManager::CreateSwapchains() {
    if (!m_session) return false;

    DestroySwapchains();

    int64_t chosenFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    if (xrEnumerateSwapchainFormats) {
        uint32_t formatCount = 0;
        xrEnumerateSwapchainFormats(m_session, 0, &formatCount, nullptr);
        if (formatCount > 0) {
            std::vector<int64_t> formats(formatCount);
            xrEnumerateSwapchainFormats(m_session, formatCount, &formatCount, formats.data());

            bool foundPreferred = false;
            for (auto f : formats) {
                if (f == DXGI_FORMAT_R8G8B8A8_UNORM) {
                    chosenFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
                    foundPreferred = true;
                    break;
                }
            }
            if (!foundPreferred) {
                for (auto f : formats) {
                    if (f == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB || f == DXGI_FORMAT_B8G8R8A8_UNORM) {
                        chosenFormat = f;
                        break;
                    }
                }
            }
        }
    }

    for (int eye = 0; eye < 2; ++eye) {
        XrSwapchainCreateInfo createInfo{XR_TYPE_SWAPCHAIN_CREATE_INFO};
        createInfo.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT | 
                                XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT | 
                                XR_SWAPCHAIN_USAGE_SAMPLED_BIT;
        createInfo.format = chosenFormat;
        createInfo.sampleCount = 1;
        createInfo.width = m_recommendedWidth;
        createInfo.height = m_recommendedHeight;
        createInfo.faceCount = 1;
        createInfo.arraySize = 1;
        createInfo.mipCount = 1;

        XrResult res = xrCreateSwapchain(m_session, &createInfo, &m_swapchains[eye].handle);
        if (XR_FAILED(res)) {
            std::cerr << "[Host] ERROR: xrCreateSwapchain for eye " << eye << " failed (" << GetXrErrorString(res) << ")" << std::endl;
            return false;
        }

        m_swapchains[eye].width = m_recommendedWidth;
        m_swapchains[eye].height = m_recommendedHeight;

        uint32_t imageCount = 0;
        xrEnumerateSwapchainImages(m_swapchains[eye].handle, 0, &imageCount, nullptr);
        m_swapchains[eye].images.resize(imageCount, {XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR});
        xrEnumerateSwapchainImages(m_swapchains[eye].handle, imageCount, &imageCount, (XrSwapchainImageBaseHeader*)m_swapchains[eye].images.data());
    }

    std::cout << "[Host] Created dual eye OpenXR D3D11 swapchains (" << m_recommendedWidth << "x" << m_recommendedHeight << ")" << std::endl;
    return true;
}

void OpenXRManager::DestroySwapchains() {
    for (int eye = 0; eye < 2; ++eye) {
        if (m_swapchains[eye].handle != XR_NULL_HANDLE && xrDestroySwapchain) {
            xrDestroySwapchain(m_swapchains[eye].handle);
            m_swapchains[eye].handle = XR_NULL_HANDLE;
        }
        m_swapchains[eye].images.clear();
    }
}

bool OpenXRManager::TryConnectOpenXR() {
    if (m_sessionActive) return true;

    if (!m_openxrAvailable) {
        if (!LoadLoader()) return false;
    }

    if (!m_instance) {
        if (!CreateInstance()) {
            return false;
        }
    }

    if (!m_session) {
        if (!InitializeSession()) {
            TeardownSession();
            return false;
        }
    }

    return true;
}

void OpenXRManager::TeardownSession() {
    ReleaseSharedTextures();
    DestroySwapchains();

    if (m_leftGripSpace && xrDestroySpace) { xrDestroySpace(m_leftGripSpace); m_leftGripSpace = XR_NULL_HANDLE; }
    if (m_rightGripSpace && xrDestroySpace) { xrDestroySpace(m_rightGripSpace); m_rightGripSpace = XR_NULL_HANDLE; }
    if (m_leftAimSpace && xrDestroySpace) { xrDestroySpace(m_leftAimSpace); m_leftAimSpace = XR_NULL_HANDLE; }
    if (m_rightAimSpace && xrDestroySpace) { xrDestroySpace(m_rightAimSpace); m_rightAimSpace = XR_NULL_HANDLE; }
    if (m_appSpace && xrDestroySpace) { xrDestroySpace(m_appSpace); m_appSpace = XR_NULL_HANDLE; }
    if (m_viewSpace && xrDestroySpace) { xrDestroySpace(m_viewSpace); m_viewSpace = XR_NULL_HANDLE; }

    if (m_actionSet && xrDestroyActionSet) {
        xrDestroyActionSet(m_actionSet);
        m_actionSet = XR_NULL_HANDLE;
    }

    if (m_session && xrDestroySession) {
        xrDestroySession(m_session);
        m_session = XR_NULL_HANDLE;
    }

    if (m_instance && xrDestroyInstance) {
        xrDestroyInstance(m_instance);
        m_instance = XR_NULL_HANDLE;
    }

    m_sessionActive = false;
    m_sessionState = XR_SESSION_STATE_UNKNOWN;
    std::cout << "[Host] OpenXR session cleared. Ready for reconnection." << std::endl;
}

bool OpenXRManager::Initialize() {
    if (m_initialized) return true;

    std::cout << "[Host] Initializing Direct3D 11 & OpenXR runtime bridge..." << std::endl;

    InitializeD3D11Device();

    if (LoadLoader()) {
        std::cout << "[Host] Successfully linked openxr_loader.dll." << std::endl;
        TryConnectOpenXR();
    } else {
        std::cout << "[Host] Notice: openxr_loader.dll not active; running in low-latency IPC standalone bridge mode." << std::endl;
    }

    m_initialized = true;
    m_running = true;
    return true;
}

bool OpenXRManager::Validate(std::string& outReport) {
    std::ostringstream ss;
    ss << "========================================================\n";
    ss << "         Riddick VR - OpenXR Validation Suite           \n";
    ss << "========================================================\n";

    bool loaderOk = LoadLoader();
    ss << "[1/4] OpenXR Loader:         " << (loaderOk ? "PRESENT (openxr_loader.dll)" : "NOT FOUND") << "\n";

    if (!loaderOk) {
        ss << "      Status: OpenXR runtime loader library is missing from PATH.\n";
        outReport = ss.str();
        return false;
    }

    bool d3dOk = InitializeD3D11Device();
    ss << "[2/4] Direct3D 11 Device:    " << (d3dOk ? "READY (Hardware FL 11.0)" : "FAILED") << "\n";

    bool instOk = CreateInstance();
    ss << "[3/4] OpenXR Instance:       " << (instOk ? "CREATED (Riddick VR / XR_KHR_D3D11_ENABLE)" : "FAILED / RUNTIME INACTIVE") << "\n";

    if (instOk && m_instance) {
        XrInstanceProperties instProps{XR_TYPE_INSTANCE_PROPERTIES};
        if (XR_SUCCEEDED(xrGetInstanceProperties(m_instance, &instProps))) {
            ss << "      Runtime Name:          " << instProps.runtimeName << "\n";
            ss << "      Runtime Version:       " 
               << XR_VERSION_MAJOR(instProps.runtimeVersion) << "." 
               << XR_VERSION_MINOR(instProps.runtimeVersion) << "." 
               << XR_VERSION_PATCH(instProps.runtimeVersion) << "\n";
        }

        XrSystemGetInfo sysInfo{XR_TYPE_SYSTEM_GET_INFO};
        sysInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
        XrResult sysRes = xrGetSystem(m_instance, &sysInfo, &m_systemId);
        bool hmdFound = XR_SUCCEEDED(sysRes);
        ss << "[4/4] Headset Discovery:     " << (hmdFound ? "DETECTED (Form Factor: HMD)" : "NOT CONNECTED") << "\n";

        if (hmdFound) {
            XrSystemProperties sysProps{XR_TYPE_SYSTEM_PROPERTIES};
            xrGetSystemProperties(m_instance, m_systemId, &sysProps);
            ss << "      System Name:           " << sysProps.systemName << "\n";
            ss << "      Vendor ID:             0x" << std::hex << sysProps.vendorId << std::dec << "\n";

            uint32_t viewCount = 0;
            xrEnumerateViewConfigurationViews(m_instance, m_systemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 0, &viewCount, nullptr);
            if (viewCount >= 2) {
                std::vector<XrViewConfigurationView> views(viewCount, {XR_TYPE_VIEW_CONFIGURATION_VIEW});
                xrEnumerateViewConfigurationViews(m_instance, m_systemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, viewCount, &viewCount, views.data());
                ss << "      Stereo Recommended:    " << views[0].recommendedImageRectWidth << "x" << views[0].recommendedImageRectHeight << "\n";
                ss << "      Stereo Max Resolution: " << views[0].maxImageRectWidth << "x" << views[0].maxImageRectHeight << "\n";
            }
        }
    } else {
        ss << "[4/4] Headset Discovery:     SKIPPED (No active OpenXR runtime)\n";
    }

    ss << "--------------------------------------------------------\n";
    ss << "Overall Diagnostic Result:   " << ((loaderOk && d3dOk) ? "PASS / READY FOR VR" : "CONFIG ERROR") << "\n";
    ss << "========================================================\n";

    outReport = ss.str();
    return (loaderOk && d3dOk);
}

void OpenXRManager::Shutdown() {
    TeardownSession();

    if (m_hLoader) {
        FreeLibrary(m_hLoader);
        m_hLoader = nullptr;
    }

    if (m_d3d11Context) {
        m_d3d11Context->Release();
        m_d3d11Context = nullptr;
    }
    if (m_d3d11Device) {
        m_d3d11Device->Release();
        m_d3d11Device = nullptr;
    }

    m_running = false;
    m_sessionActive = false;
    m_initialized = false;
    std::cout << "[Host] OpenXR Manager cleanly shut down." << std::endl;
}

bool OpenXRManager::EnsureSharedTextures(uint64_t leftHandle, uint64_t rightHandle) {
    if (!m_d3d11Device || !leftHandle || !rightHandle) return false;
    if (leftHandle == m_currentLeftHandle && rightHandle == m_currentRightHandle && m_sharedLeftTex && m_sharedRightTex) {
        return true;
    }

    ReleaseSharedTextures();

    HANDLE hLeft = reinterpret_cast<HANDLE>(static_cast<uintptr_t>(leftHandle & 0xFFFFFFFFULL));
    HANDLE hRight = reinterpret_cast<HANDLE>(static_cast<uintptr_t>(rightHandle & 0xFFFFFFFFULL));

    HRESULT hrLeft = m_d3d11Device->OpenSharedResource(
        hLeft,
        __uuidof(ID3D11Texture2D),
        (void**)&m_sharedLeftTex
    );

    HRESULT hrRight = m_d3d11Device->OpenSharedResource(
        hRight,
        __uuidof(ID3D11Texture2D),
        (void**)&m_sharedRightTex
    );

    if (SUCCEEDED(hrLeft) && SUCCEEDED(hrRight) && m_sharedLeftTex && m_sharedRightTex) {
        m_currentLeftHandle = leftHandle;
        m_currentRightHandle = rightHandle;
        return true;
    } else {
        ReleaseSharedTextures();
        return false;
    }
}

void OpenXRManager::ReleaseSharedTextures() {
    if (m_sharedLeftTex) {
        m_sharedLeftTex->Release();
        m_sharedLeftTex = nullptr;
    }
    if (m_sharedRightTex) {
        m_sharedRightTex->Release();
        m_sharedRightTex = nullptr;
    }
    m_currentLeftHandle = 0;
    m_currentRightHandle = 0;
}

bool OpenXRManager::PollEvents() {
    if (!m_instance || !xrPollEvent) return m_running;

    XrEventDataBuffer eventBuffer{XR_TYPE_EVENT_DATA_BUFFER};
    while (xrPollEvent(m_instance, &eventBuffer) == XR_SUCCESS) {
        if (eventBuffer.type == XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED) {
            auto* sessionStateChanged = (XrEventDataSessionStateChanged*)&eventBuffer;
            m_sessionState = sessionStateChanged->state;
            if (m_sessionState == XR_SESSION_STATE_READY) {
                XrSessionBeginInfo beginInfo{XR_TYPE_SESSION_BEGIN_INFO};
                beginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
                XrResult res = xrBeginSession(m_session, &beginInfo);
                if (XR_SUCCEEDED(res)) {
                    m_sessionActive = true;
                    std::cout << "[Host] >>> OpenXR Session READY -> Switched to active VR rendering! <<<" << std::endl;
                } else {
                    std::cout << "[Host] Notice: xrBeginSession failed (" << GetXrErrorString(res) << ")" << std::endl;
                }
            } else if (m_sessionState == XR_SESSION_STATE_STOPPING) {
                std::cout << "[Host] OpenXR Session stopping..." << std::endl;
                if (m_sessionActive && xrEndSession) {
                    xrEndSession(m_session);
                }
                m_sessionActive = false;
            } else if (m_sessionState == XR_SESSION_STATE_EXITING || m_sessionState == XR_SESSION_STATE_LOSS_PENDING) {
                std::cout << "[Host] OpenXR Session lost/exiting. Resetting session for reconnection..." << std::endl;
                TeardownSession();
                break;
            }
        }
        eventBuffer.type = XR_TYPE_EVENT_DATA_BUFFER;
    }
    return m_running;
}

void OpenXRManager::UpdateTracking(RiddickVRSharedState& state, XrTime predictedTime) {
    m_frameIndex++;
    state.frameIndex = m_frameIndex;
    state.vrActive = m_sessionActive;
    state.renderWidth = m_recommendedWidth;
    state.renderHeight = m_recommendedHeight;

    if (m_sessionActive && m_session && m_appSpace) {
        // Sync Actions
        XrActiveActionSet activeSet{m_actionSet, XR_NULL_PATH};
        XrActionsSyncInfo syncInfo{XR_TYPE_ACTIONS_SYNC_INFO};
        syncInfo.countActiveActionSets = 1;
        syncInfo.activeActionSets = &activeSet;
        xrSyncActions(m_session, &syncInfo);

        // Read Locomotion & Turning
        XrActionStateGetInfo getInfo{XR_TYPE_ACTION_STATE_GET_INFO};

        getInfo.action = m_locomotionAction;
        getInfo.subactionPath = m_leftHandPath;
        XrActionStateVector2f locoState{XR_TYPE_ACTION_STATE_VECTOR2F};
        if (XR_SUCCEEDED(xrGetActionStateVector2f(m_session, &getInfo, &locoState)) && locoState.isActive) {
            state.leftController.thumbstickX = locoState.currentState.x;
            state.leftController.thumbstickY = locoState.currentState.y;
        }

        getInfo.action = m_turnAction;
        getInfo.subactionPath = m_rightHandPath;
        XrActionStateVector2f turnState{XR_TYPE_ACTION_STATE_VECTOR2F};
        if (XR_SUCCEEDED(xrGetActionStateVector2f(m_session, &getInfo, &turnState)) && turnState.isActive) {
            state.rightController.thumbstickX = turnState.currentState.x;
            state.rightController.thumbstickY = turnState.currentState.y;
        }

        // Read Triggers
        getInfo.action = m_triggerAction;
        getInfo.subactionPath = m_leftHandPath;
        XrActionStateFloat leftTrig{XR_TYPE_ACTION_STATE_FLOAT};
        if (XR_SUCCEEDED(xrGetActionStateFloat(m_session, &getInfo, &leftTrig))) {
            state.leftController.trigger = leftTrig.currentState;
        }

        getInfo.subactionPath = m_rightHandPath;
        XrActionStateFloat rightTrig{XR_TYPE_ACTION_STATE_FLOAT};
        if (XR_SUCCEEDED(xrGetActionStateFloat(m_session, &getInfo, &rightTrig))) {
            state.rightController.trigger = rightTrig.currentState;
        }

        // Read Grips
        getInfo.action = m_gripAction;
        getInfo.subactionPath = m_leftHandPath;
        XrActionStateFloat leftGrip{XR_TYPE_ACTION_STATE_FLOAT};
        if (XR_SUCCEEDED(xrGetActionStateFloat(m_session, &getInfo, &leftGrip))) {
            state.leftController.grip = leftGrip.currentState;
        }

        getInfo.subactionPath = m_rightHandPath;
        XrActionStateFloat rightGrip{XR_TYPE_ACTION_STATE_FLOAT};
        if (XR_SUCCEEDED(xrGetActionStateFloat(m_session, &getInfo, &rightGrip))) {
            state.rightController.grip = rightGrip.currentState;
        }

        // Read Buttons
        state.leftController.buttons = 0;
        state.rightController.buttons = 0;

        getInfo.action = m_primaryBtnAction;
        getInfo.subactionPath = m_rightHandPath;
        XrActionStateBoolean btnA{XR_TYPE_ACTION_STATE_BOOLEAN};
        if (XR_SUCCEEDED(xrGetActionStateBoolean(m_session, &getInfo, &btnA)) && btnA.currentState) {
            state.rightController.buttons |= RIDDICK_VR_BTN_A;
        }

        getInfo.action = m_secondaryBtnAction;
        getInfo.subactionPath = m_rightHandPath;
        XrActionStateBoolean btnB{XR_TYPE_ACTION_STATE_BOOLEAN};
        if (XR_SUCCEEDED(xrGetActionStateBoolean(m_session, &getInfo, &btnB)) && btnB.currentState) {
            state.rightController.buttons |= RIDDICK_VR_BTN_B;
        }

        getInfo.action = m_primaryBtnAction;
        getInfo.subactionPath = m_leftHandPath;
        XrActionStateBoolean btnX{XR_TYPE_ACTION_STATE_BOOLEAN};
        if (XR_SUCCEEDED(xrGetActionStateBoolean(m_session, &getInfo, &btnX)) && btnX.currentState) {
            state.leftController.buttons |= RIDDICK_VR_BTN_X;
        }

        getInfo.action = m_secondaryBtnAction;
        getInfo.subactionPath = m_leftHandPath;
        XrActionStateBoolean btnY{XR_TYPE_ACTION_STATE_BOOLEAN};
        if (XR_SUCCEEDED(xrGetActionStateBoolean(m_session, &getInfo, &btnY)) && btnY.currentState) {
            state.leftController.buttons |= RIDDICK_VR_BTN_Y;
        }

        // Recenter
        getInfo.action = m_recenterBtnAction;
        getInfo.subactionPath = m_rightHandPath;
        XrActionStateBoolean btnRecenter{XR_TYPE_ACTION_STATE_BOOLEAN};
        if (XR_SUCCEEDED(xrGetActionStateBoolean(m_session, &getInfo, &btnRecenter)) && btnRecenter.currentState) {
            state.shouldRecenter = true;
        }

        // Apply Haptics if requested
        if (state.leftHapticPulse > 0.05f && xrApplyHapticFeedback) {
            XrHapticVibration vib{XR_TYPE_HAPTIC_VIBRATION};
            vib.amplitude = state.leftHapticPulse;
            vib.duration = XR_MIN_HAPTIC_DURATION * 2;
            vib.frequency = XR_FREQUENCY_UNSPECIFIED;
            XrHapticActionInfo hapticInfo{XR_TYPE_HAPTIC_ACTION_INFO};
            hapticInfo.action = m_hapticAction;
            hapticInfo.subactionPath = m_leftHandPath;
            xrApplyHapticFeedback(m_session, &hapticInfo, (XrHapticBaseHeader*)&vib);
        }
        if (state.rightHapticPulse > 0.05f && xrApplyHapticFeedback) {
            XrHapticVibration vib{XR_TYPE_HAPTIC_VIBRATION};
            vib.amplitude = state.rightHapticPulse;
            vib.duration = XR_MIN_HAPTIC_DURATION * 2;
            vib.frequency = XR_FREQUENCY_UNSPECIFIED;
            XrHapticActionInfo hapticInfo{XR_TYPE_HAPTIC_ACTION_INFO};
            hapticInfo.action = m_hapticAction;
            hapticInfo.subactionPath = m_rightHandPath;
            xrApplyHapticFeedback(m_session, &hapticInfo, (XrHapticBaseHeader*)&vib);
        }

        // Locate Controller Poses and compute linear velocities
        static RiddickVRVector3 s_prevLeftPos{0, 0, 0};
        static RiddickVRVector3 s_prevRightPos{0, 0, 0};
        static auto s_prevTime = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - s_prevTime).count();
        if (dt < 0.001f) dt = 0.016f;

        auto LocateTrackedPose = [&](XrSpace space, RiddickVRTrackedPose& outPose, RiddickVRVector3& prevPos) {
            XrSpaceLocation loc{XR_TYPE_SPACE_LOCATION};
            XrSpaceVelocity vel{XR_TYPE_SPACE_VELOCITY};
            loc.next = &vel;
            if (XR_SUCCEEDED(xrLocateSpace(space, m_appSpace, predictedTime, &loc)) &&
                (loc.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT)) {
                outPose.position = { loc.pose.position.x, loc.pose.position.y, loc.pose.position.z };
                outPose.orientation = { loc.pose.orientation.x, loc.pose.orientation.y, loc.pose.orientation.z, loc.pose.orientation.w };
                outPose.isValid = true;

                if (vel.velocityFlags & XR_SPACE_VELOCITY_LINEAR_VALID_BIT) {
                    outPose.linearVelocity = { vel.linearVelocity.x, vel.linearVelocity.y, vel.linearVelocity.z };
                } else {
                    outPose.linearVelocity = {
                        (outPose.position.x - prevPos.x) / dt,
                        (outPose.position.y - prevPos.y) / dt,
                        (outPose.position.z - prevPos.z) / dt
                    };
                }
                prevPos = outPose.position;
            }
        };

        if (m_leftGripSpace) LocateTrackedPose(m_leftGripSpace, state.leftController.gripPose, s_prevLeftPos);
        if (m_rightGripSpace) LocateTrackedPose(m_rightGripSpace, state.rightController.gripPose, s_prevRightPos);
        s_prevTime = now;

        state.leftController.isConnected = state.leftController.gripPose.isValid;
        state.rightController.isConnected = state.rightController.gripPose.isValid;

        // Locate Stereo Views & Projection
        XrViewLocateInfo viewLocateInfo{XR_TYPE_VIEW_LOCATE_INFO};
        viewLocateInfo.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
        viewLocateInfo.displayTime = predictedTime;
        viewLocateInfo.space = m_appSpace;

        XrViewState viewState{XR_TYPE_VIEW_STATE};
        uint32_t viewCount = 0;
        XrView views[2] = { {XR_TYPE_VIEW}, {XR_TYPE_VIEW} };

        if (XR_SUCCEEDED(xrLocateViews(m_session, &viewLocateInfo, &viewState, 2, &viewCount, views)) && viewCount >= 2) {
            m_locatedViews[0] = views[0];
            m_locatedViews[1] = views[1];
            state.leftEyeProjection = CreateProjectionFromFov(views[0].fov);
            state.rightEyeProjection = CreateProjectionFromFov(views[1].fov);

            state.leftEyeView = CreateViewFromPose(views[0].pose);
            state.rightEyeView = CreateViewFromPose(views[1].pose);

            state.hmdPose.position = { views[0].pose.position.x, views[0].pose.position.y, views[0].pose.position.z };
            state.hmdPose.orientation = { views[0].pose.orientation.x, views[0].pose.orientation.y, views[0].pose.orientation.z, views[0].pose.orientation.w };
            state.hmdPose.isValid = true;
            return;
        }
    }

    // Fallback: Default synthetic 65mm IPD baseline matrices
    state.hmdPose.orientation = {0.0f, 0.0f, 0.0f, 1.0f};
    state.hmdPose.position = {0.0f, 0.0f, 0.0f};
    state.hmdPose.isValid = true;

    constexpr float HALF_IPD = 0.0325f;
    for (int i = 0; i < 16; ++i) {
        state.leftEyeView.m[i] = (i % 5 == 0) ? 1.0f : 0.0f;
        state.rightEyeView.m[i] = (i % 5 == 0) ? 1.0f : 0.0f;
    }
    state.leftEyeView.m[12] = -HALF_IPD;
    state.rightEyeView.m[12] = +HALF_IPD;

    constexpr float fovTan = 1.0f;
    constexpr float nearZ = 0.05f;
    constexpr float farZ = 1000.0f;
    for (int i = 0; i < 16; ++i) {
        state.leftEyeProjection.m[i] = 0.0f;
        state.rightEyeProjection.m[i] = 0.0f;
    }
    state.leftEyeProjection.m[0] = 1.0f / fovTan;
    state.leftEyeProjection.m[5] = 1.0f / fovTan;
    state.leftEyeProjection.m[10] = -(farZ + nearZ) / (farZ - nearZ);
    state.leftEyeProjection.m[11] = -1.0f;
    state.leftEyeProjection.m[14] = -(2.0f * farZ * nearZ) / (farZ - nearZ);
    state.rightEyeProjection = state.leftEyeProjection;

    state.leftController.isConnected = true;
    state.rightController.isConnected = true;
}

void OpenXRManager::RenderFrame(IpcBridgeHost& ipcHost) {
    XrFrameState frameState{XR_TYPE_FRAME_STATE};
    XrTime predictedDisplayTime = 0;

    if (m_sessionActive && m_session && xrWaitFrame) {
        XrFrameWaitInfo waitInfo{XR_TYPE_FRAME_WAIT_INFO};
        xrWaitFrame(m_session, &waitInfo, &frameState);
        predictedDisplayTime = frameState.predictedDisplayTime;

        XrFrameBeginInfo beginInfo{XR_TYPE_FRAME_BEGIN_INFO};
        xrBeginFrame(m_session, &beginInfo);
    }

    RiddickVRSharedState state{};
    UpdateTracking(state, predictedDisplayTime);

    // Transmit updated tracking to the game hook
    ipcHost.WriteState(state);

    // Wait for the game hook to finish rendering the frame (up to 16ms)
    bool frameReady = ipcHost.WaitForFrameReady(16);
    if (frameReady) {
        RiddickVRSharedState clientState{};
        if (ipcHost.ReadState(clientState)) {
            if (clientState.leftEyeSharedHandle && clientState.rightEyeSharedHandle) {
                EnsureSharedTextures(clientState.leftEyeSharedHandle, clientState.rightEyeSharedHandle);
            }
        }
    }

    // Submit OpenXR compositor frame if session is active
    if (m_sessionActive && m_session && xrEndFrame) {
        std::vector<XrCompositionLayerBaseHeader*> layers;
        XrCompositionLayerProjection layer{XR_TYPE_COMPOSITION_LAYER_PROJECTION};
        XrCompositionLayerProjectionView projViews[2] = {
            {XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW},
            {XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW}
        };

        if (frameState.shouldRender && m_swapchains[0].handle && m_swapchains[1].handle) {
            for (int eye = 0; eye < 2; ++eye) {
                uint32_t imageIndex = 0;
                XrSwapchainImageAcquireInfo acqInfo{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
                xrAcquireSwapchainImage(m_swapchains[eye].handle, &acqInfo, &imageIndex);

                XrSwapchainImageWaitInfo waitImageInfo{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
                waitImageInfo.timeout = XR_INFINITE_DURATION;
                xrWaitSwapchainImage(m_swapchains[eye].handle, &waitImageInfo);

                // Copy shared D3D11 texture into swapchain image if available
                ID3D11Texture2D* srcTex = (eye == 0) ? m_sharedLeftTex : m_sharedRightTex;
                if (srcTex && m_d3d11Context && imageIndex < m_swapchains[eye].images.size()) {
                    m_d3d11Context->CopyResource(m_swapchains[eye].images[imageIndex].texture, srcTex);
                }

                XrSwapchainImageReleaseInfo relInfo{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
                xrReleaseSwapchainImage(m_swapchains[eye].handle, &relInfo);

                projViews[eye].pose = m_locatedViews[eye].pose;
                projViews[eye].fov = m_locatedViews[eye].fov;
                projViews[eye].subImage.swapchain = m_swapchains[eye].handle;
                projViews[eye].subImage.imageRect.offset = {0, 0};
                projViews[eye].subImage.imageRect.extent = { (int32_t)m_swapchains[eye].width, (int32_t)m_swapchains[eye].height };
            }

            layer.space = m_appSpace;
            layer.viewCount = 2;
            layer.views = projViews;
            layers.push_back((XrCompositionLayerBaseHeader*)&layer);
        }

        XrFrameEndInfo endInfo{XR_TYPE_FRAME_END_INFO};
        endInfo.displayTime = predictedDisplayTime;
        endInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
        endInfo.layerCount = (uint32_t)layers.size();
        endInfo.layers = layers.data();
        xrEndFrame(m_session, &endInfo);
    }

    if (frameReady) {
        ipcHost.SignalFrameConsumed();
    }
}

void OpenXRManager::RunLoop(IpcBridgeHost& ipcHost, bool autoExitWithGame) {
    std::cout << "[Host] Entering VR compositor main loop. Press Ctrl+C in console to terminate." << std::endl;

    auto lastReport = std::chrono::steady_clock::now();
    auto lastRetry = std::chrono::steady_clock::now();
    auto lastClientSeen = std::chrono::steady_clock::now();
    bool hadClient = false;
    uint32_t framesCounted = 0;

    while (m_running && PollEvents()) {
        auto now = std::chrono::steady_clock::now();

        if (ipcHost.IsClientConnected()) {
            hadClient = true;
            lastClientSeen = now;
        } else if (autoExitWithGame && hadClient) {
            if (std::chrono::duration_cast<std::chrono::seconds>(now - lastClientSeen).count() >= 4) {
                std::cout << "[Host] Game process disconnected. Auto-exiting host companion..." << std::endl;
                break;
            }
        }

        // Retry connecting to OpenXR runtime periodically if not yet active
        if (!m_sessionActive || m_session == XR_NULL_HANDLE) {
            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastRetry).count() >= 1500) {
                lastRetry = now;
                TryConnectOpenXR();
            }
        }

        RenderFrame(ipcHost);
        framesCounted++;

        if (std::chrono::duration_cast<std::chrono::seconds>(now - lastReport).count() >= 5) {
            float fps = framesCounted / 5.0f;
            std::cout << "[Host] Heartbeat: " << std::fixed << std::setprecision(1) << fps << " FPS | Connected: " 
                      << (ipcHost.IsClientConnected() ? "YES" : "WAITING FOR GAME")
                      << " | OpenXR Session: " << (m_sessionActive ? "RUNNING (IMMERSIVE VR)" : "SEARCHING FOR HEADSET...") << std::endl;
            framesCounted = 0;
            lastReport = now;
        }

        if (!ipcHost.IsClientConnected() && !m_sessionActive) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

} // namespace RiddickVR
