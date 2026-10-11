#pragma once

#include <stdint.h>

#define RIDDICK_VR_IPC_SHARED_MEM_NAME "Local\\RiddickVR_SharedMemory"
#define RIDDICK_VR_IPC_FRAME_READY_EVENT "Local\\RiddickVR_FrameReady"
#define RIDDICK_VR_IPC_FRAME_CONSUMED_EVENT "Local\\RiddickVR_FrameConsumed"
#define RIDDICK_VR_IPC_MUTEX_NAME "Local\\RiddickVR_Mutex"

#define RIDDICK_VR_PROTOCOL_VERSION 1

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
    
    // Texture sharing parameters
    uint32_t renderWidth;
    uint32_t renderHeight;
    uint64_t leftEyeSharedHandle;
    uint64_t rightEyeSharedHandle;
    
    // Flags and heartbeat
    bool vrActive;
    bool shouldRecenter;
    bool clientConnected;
    uint32_t clientFrameCount;
    uint32_t heartbeatTimestamp;
};

#pragma pack(pop)
