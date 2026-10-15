#pragma once

#include <stdint.h>

#define RIDDICK_VR_IPC_SHARED_MEM_NAME "Local\\RiddickVR_SharedMemory"
#define RIDDICK_VR_IPC_FRAME_READY_EVENT "Local\\RiddickVR_FrameReady"
#define RIDDICK_VR_IPC_FRAME_CONSUMED_EVENT "Local\\RiddickVR_FrameConsumed"
#define RIDDICK_VR_IPC_MUTEX_NAME "Local\\RiddickVR_Mutex"

#define RIDDICK_VR_PROTOCOL_VERSION 1
// GH #28: version negotiation range. v1 layout is frozen; newer hosts must
// still accept readers/writers advertising MIN..CURRENT. Bump CURRENT only
// with a layout change, and keep MIN at the oldest layout the host can parse.
#define RIDDICK_VR_PROTOCOL_MIN_SUPPORTED 1

constexpr uint32_t RIDDICK_VR_BTN_A           = 1 << 0;
constexpr uint32_t RIDDICK_VR_BTN_B           = 1 << 1;
constexpr uint32_t RIDDICK_VR_BTN_X           = 1 << 2;
constexpr uint32_t RIDDICK_VR_BTN_Y           = 1 << 3;
constexpr uint32_t RIDDICK_VR_BTN_STICK_LEFT  = 1 << 4;
constexpr uint32_t RIDDICK_VR_BTN_STICK_RIGHT = 1 << 5;
constexpr uint32_t RIDDICK_VR_BTN_MENU        = 1 << 6;
constexpr uint32_t RIDDICK_VR_TOUCH_TRIGGER    = 1 << 7;
constexpr uint32_t RIDDICK_VR_TOUCH_THUMBSTICK = 1 << 8;
constexpr uint32_t RIDDICK_VR_TOUCH_THUMBREST  = 1 << 9;

#pragma pack(push, 4)

struct RiddickVRMatrix4x4 {
    float m[16]; // Column-major 4x4 matrix
};

struct RiddickVRVector3 {
    float x, y, z;
};

struct RiddickVRQuaternion {
    float x, y, z, w;
};

struct RiddickVRTrackedPose {
    RiddickVRVector3 position;
    RiddickVRQuaternion orientation;
    RiddickVRVector3 linearVelocity;
    RiddickVRVector3 angularVelocity;
    bool isValid;
};

struct RiddickVRControllerInput {
    bool isConnected;
    RiddickVRTrackedPose gripPose;
    RiddickVRTrackedPose aimPose;
    float trigger;       // 0.0 to 1.0
    float grip;          // 0.0 to 1.0
    float thumbstickX;   // -1.0 to 1.0
    float thumbstickY;   // -1.0 to 1.0
    uint32_t buttons;    // Bitmask for buttons
};

struct RiddickVRSharedState {
    uint32_t protocolVersion;
    uint32_t frameIndex;
    
    // Headset tracking & projection from Host -> Hook
    RiddickVRTrackedPose hmdPose;
    RiddickVRMatrix4x4 leftEyeView;
    RiddickVRMatrix4x4 leftEyeProjection;
    RiddickVRMatrix4x4 rightEyeView;
    RiddickVRMatrix4x4 rightEyeProjection;
    
    // Motion controllers from Host -> Hook
    RiddickVRControllerInput leftController;
    RiddickVRControllerInput rightController;
    
    // Haptic feedback from Hook -> Host
    float leftHapticPulse;   // 0.0 to 1.0 amplitude
    float rightHapticPulse;  // 0.0 to 1.0 amplitude
    
    // Texture sharing parameters
    uint32_t renderWidth;
    uint32_t renderHeight;
    uint64_t leftEyeSharedHandle;
    uint64_t rightEyeSharedHandle;
    
    // Flags and heartbeat
    bool vrActive;
    bool shouldRecenter;
    bool clientConnected;
    bool cinemaMode;         // Cinema screen mode for menus & cutscenes
    bool eyeshineActive;     // Riddick Eyeshine night vision mode active (ADR 0022)
    bool flashlightActive;   // Tactical shoulder flashlight active
    uint16_t reservedFlags;  // 4-byte alignment padding
    uint32_t clientFrameCount;
    uint32_t heartbeatTimestamp;
};

#pragma pack(pop)

// GH #28: protocol v1 layout is frozen. These asserts fail the build if a
// field is added/removed/reordered without a deliberate version bump.
static_assert(sizeof(RiddickVRTrackedPose) == 56, "RiddickVRTrackedPose layout changed; bump RIDDICK_VR_PROTOCOL_VERSION");
static_assert(sizeof(RiddickVRControllerInput) == 136, "RiddickVRControllerInput layout changed; bump RIDDICK_VR_PROTOCOL_VERSION");
static_assert(sizeof(RiddickVRSharedState) == 640, "RiddickVRSharedState layout changed; bump RIDDICK_VR_PROTOCOL_VERSION");
static_assert(RIDDICK_VR_PROTOCOL_MIN_SUPPORTED <= RIDDICK_VR_PROTOCOL_VERSION, "protocol negotiation range inverted");

