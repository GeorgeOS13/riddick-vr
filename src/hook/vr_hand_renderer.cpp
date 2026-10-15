#include "vr_hand_renderer.h"
#include "opengl_proxy.h"
#include "logging.h"
#include <cmath>
#include <algorithm>

namespace RiddickVR {

typedef void (APIENTRY *glPushAttrib_t)(GLbitfield mask);
typedef void (APIENTRY *glPopAttrib_t)(void);
typedef void (APIENTRY *glViewport_t)(GLint x, GLint y, GLsizei width, GLsizei height);
typedef void (APIENTRY *glClearDepth_t)(GLclampd depth);
typedef void (APIENTRY *glClear_t)(GLbitfield mask);
typedef void (APIENTRY *glMatrixMode_t)(GLenum mode);
typedef void (APIENTRY *glPushMatrix_t)(void);
typedef void (APIENTRY *glPopMatrix_t)(void);
typedef void (APIENTRY *glLoadMatrixf_t)(const GLfloat *m);
typedef void (APIENTRY *glMultMatrixf_t)(const GLfloat *m);
typedef void (APIENTRY *glFrustum_t)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
typedef void (APIENTRY *glEnable_t)(GLenum cap);
typedef void (APIENTRY *glDisable_t)(GLenum cap);
typedef void (APIENTRY *glDepthFunc_t)(GLenum func);
typedef void (APIENTRY *glDepthMask_t)(GLboolean flag);
typedef void (APIENTRY *glLightfv_t)(GLenum light, GLenum pname, const GLfloat *params);
typedef void (APIENTRY *glColorMaterial_t)(GLenum face, GLenum mode);
typedef void (APIENTRY *glShadeModel_t)(GLenum mode);
typedef void (APIENTRY *glCullFace_t)(GLenum mode);
typedef void (APIENTRY *glFrontFace_t)(GLenum mode);
typedef void (APIENTRY *glBegin_t)(GLenum mode);
typedef void (APIENTRY *glEnd_t)(void);
typedef void (APIENTRY *glVertex3f_t)(GLfloat x, GLfloat y, GLfloat z);
typedef void (APIENTRY *glNormal3f_t)(GLfloat nx, GLfloat ny, GLfloat nz);
typedef void (APIENTRY *glColor3f_t)(GLfloat red, GLfloat green, GLfloat blue);
typedef void (APIENTRY *glRotatef_t)(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
typedef void (APIENTRY *glTranslatef_t)(GLfloat x, GLfloat y, GLfloat z);
typedef void (APIENTRY *glGetIntegerv_t)(GLenum pname, GLint *params);

typedef void (APIENTRY *PFNGLUSEPROGRAMPROC)(GLuint program);
typedef void (APIENTRY *PFNGLBINDBUFFERPROC)(GLenum target, GLuint buffer);

struct GLDispatch {
    glPushAttrib_t glPushAttrib = nullptr;
    glPopAttrib_t glPopAttrib = nullptr;
    glViewport_t glViewport = nullptr;
    glClearDepth_t glClearDepth = nullptr;
    glClear_t glClear = nullptr;
    glMatrixMode_t glMatrixMode = nullptr;
    glPushMatrix_t glPushMatrix = nullptr;
    glPopMatrix_t glPopMatrix = nullptr;
    glLoadMatrixf_t glLoadMatrixf = nullptr;
    glMultMatrixf_t glMultMatrixf = nullptr;
    glFrustum_t glFrustum = nullptr;
    glEnable_t glEnable = nullptr;
    glDisable_t glDisable = nullptr;
    glDepthFunc_t glDepthFunc = nullptr;
    glDepthMask_t glDepthMask = nullptr;
    glLightfv_t glLightfv = nullptr;
    glColorMaterial_t glColorMaterial = nullptr;
    glShadeModel_t glShadeModel = nullptr;
    glCullFace_t glCullFace = nullptr;
    glFrontFace_t glFrontFace = nullptr;
    glBegin_t glBegin = nullptr;
    glEnd_t glEnd = nullptr;
    glVertex3f_t glVertex3f = nullptr;
    glNormal3f_t glNormal3f = nullptr;
    glColor3f_t glColor3f = nullptr;
    glRotatef_t glRotatef = nullptr;
    glTranslatef_t glTranslatef = nullptr;
    glGetIntegerv_t glGetIntegerv = nullptr;

    PFNGLUSEPROGRAMPROC glUseProgram = nullptr;
    PFNGLBINDBUFFERPROC glBindBuffer = nullptr;
    bool initialized = false;

    void Init() {
        if (initialized) return;
        HMODULE hReal = OpenGLProxy::GetRealOpenGLModule();
        if (!hReal) {
            hReal = GetModuleHandleA("opengl32.dll");
        }
        if (!hReal) return;

        #define LOAD_GL(fn) fn = (fn##_t)GetProcAddress(hReal, #fn)
        LOAD_GL(glPushAttrib);
        LOAD_GL(glPopAttrib);
        LOAD_GL(glViewport);
        LOAD_GL(glClearDepth);
        LOAD_GL(glClear);
        LOAD_GL(glMatrixMode);
        LOAD_GL(glPushMatrix);
        LOAD_GL(glPopMatrix);
        LOAD_GL(glLoadMatrixf);
        LOAD_GL(glMultMatrixf);
        LOAD_GL(glFrustum);
        LOAD_GL(glEnable);
        LOAD_GL(glDisable);
        LOAD_GL(glDepthFunc);
        LOAD_GL(glDepthMask);
        LOAD_GL(glLightfv);
        LOAD_GL(glColorMaterial);
        LOAD_GL(glShadeModel);
        LOAD_GL(glCullFace);
        LOAD_GL(glFrontFace);
        LOAD_GL(glBegin);
        LOAD_GL(glEnd);
        LOAD_GL(glVertex3f);
        LOAD_GL(glNormal3f);
        LOAD_GL(glColor3f);
        LOAD_GL(glRotatef);
        LOAD_GL(glTranslatef);
        LOAD_GL(glGetIntegerv);
        #undef LOAD_GL

        typedef PROC (WINAPI *wglGetProcAddress_t)(LPCSTR);
        wglGetProcAddress_t wglGetProc = (wglGetProcAddress_t)GetProcAddress(hReal, "wglGetProcAddress");
        if (wglGetProc) {
            glUseProgram = (PFNGLUSEPROGRAMPROC)wglGetProc("glUseProgram");
            if (!glUseProgram) glUseProgram = (PFNGLUSEPROGRAMPROC)wglGetProc("glUseProgramObjectARB");
            glBindBuffer = (PFNGLBINDBUFFERPROC)wglGetProc("glBindBuffer");
            if (!glBindBuffer) glBindBuffer = (PFNGLBINDBUFFERPROC)wglGetProc("glBindBufferARB");
        }

        initialized = (glBegin != nullptr && glVertex3f != nullptr);
    }
};

static GLDispatch s_gl;

VRHandRenderer& VRHandRenderer::Instance() {
    static VRHandRenderer s_instance;
    return s_instance;
}

void VRHandRenderer::RenderHands(const RiddickVRSharedState& state, bool isLeftEye, uint32_t eyeWidth, uint32_t eyeHeight) {
    if (state.cinemaMode) {
        return; // Hands are hidden during 2D letterboxed cinema/menu scenes
    }

    const auto& leftCtrl = state.leftController;
    const auto& rightCtrl = state.rightController;

    if (!leftCtrl.gripPose.isValid && !rightCtrl.gripPose.isValid) {
        return; // No valid motion controller tracking data available
    }

    s_gl.Init();
    if (!s_gl.initialized) return;

    // 1. Push complete OpenGL pipeline attributes to preserve game render state
    s_gl.glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT |
                      GL_LIGHTING_BIT | GL_POLYGON_BIT | GL_VIEWPORT_BIT);

    // Save previous GLSL program and unbind to allow clean fixed-function tactical glove rendering
    GLint prevProgram = 0;
    if (s_gl.glGetIntegerv) {
        s_gl.glGetIntegerv(0x8B8D /* GL_CURRENT_PROGRAM */, &prevProgram);
    }
    if (s_gl.glUseProgram) {
        s_gl.glUseProgram(0);
    }
    if (s_gl.glBindBuffer) {
        s_gl.glBindBuffer(0x8892 /* GL_ARRAY_BUFFER */, 0);
        s_gl.glBindBuffer(0x8893 /* GL_ELEMENT_ARRAY_BUFFER */, 0);
    }

    // 2. Set viewport to match the active eye framebuffer
    s_gl.glViewport(0, 0, eyeWidth, eyeHeight);

    // 3. Clear depth buffer for the hand overlay pass (ensures hands never clip through distant geometry)
    s_gl.glClearDepth(1.0f);
    s_gl.glClear(GL_DEPTH_BUFFER_BIT);

    // 4. Configure stereoscopic projection matrix
    s_gl.glMatrixMode(GL_PROJECTION);
    s_gl.glPushMatrix();

    RiddickVRMatrix4x4 projMat = isLeftEye ? state.leftEyeProjection : state.rightEyeProjection;
    if (projMat.m[0] > 0.05f) {
        // Invert Y focal length to match the vertically-flipped FBO orientation (ADR 0405)
        projMat.m[5] = -projMat.m[5];
        s_gl.glLoadMatrixf(projMat.m);
    } else {
        // Fallback perspective matrix (90 deg horizontal FOV, near 0.05m, far 50.0m)
        float fovRad = 1.5707963f; // 90 deg
        float aspect = static_cast<float>(eyeWidth) / static_cast<float>(eyeHeight);
        float nearZ = 0.05f;
        float farZ = 50.0f;
        float top = nearZ * tanf(fovRad * 0.5f);
        float bottom = -top;
        float right = top * aspect;
        float left = -right;
        // Invert top and bottom for FBO top-down alignment
        s_gl.glFrustum(left, right, top, bottom, nearZ, farZ);
    }

    // 5. Configure view & lighting state
    s_gl.glMatrixMode(GL_MODELVIEW);
    s_gl.glPushMatrix();

    const RiddickVRMatrix4x4& viewMat = isLeftEye ? state.leftEyeView : state.rightEyeView;
    s_gl.glLoadMatrixf(viewMat.m);

    // Enable high-fidelity depth testing
    s_gl.glEnable(GL_DEPTH_TEST);
    s_gl.glDepthFunc(GL_LEQUAL);
    s_gl.glDepthMask(GL_TRUE);

    // Tactical directional lighting
    s_gl.glEnable(GL_LIGHTING);
    s_gl.glEnable(GL_LIGHT0);
    GLfloat lightPos[] = { 0.25f, 1.20f, 0.40f, 0.0f }; // Overhead-forward key light
    GLfloat lightDiffuse[] = { 0.90f, 0.90f, 0.92f, 1.0f };
    GLfloat lightAmbient[] = { 0.32f, 0.32f, 0.35f, 1.0f };
    GLfloat lightSpecular[] = { 0.45f, 0.45f, 0.50f, 1.0f };
    s_gl.glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    s_gl.glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
    s_gl.glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
    s_gl.glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular);

    s_gl.glEnable(GL_COLOR_MATERIAL);
    s_gl.glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    s_gl.glShadeModel(GL_SMOOTH);
    s_gl.glEnable(0x0BA7 /* GL_NORMALIZE */);

    // Enable backface culling (CW front face due to inverted Y projection)
    s_gl.glEnable(GL_CULL_FACE);
    s_gl.glFrontFace(GL_CW);
    s_gl.glCullFace(GL_BACK);
    s_gl.glDisable(GL_BLEND);
    s_gl.glDisable(GL_TEXTURE_2D);

    // Check if Boxer Guard is active (both fists clenched and raised)
    bool isBoxerGuard = (leftCtrl.grip > 0.45f && rightCtrl.grip > 0.45f &&
                         fabsf(leftCtrl.gripPose.position.x - rightCtrl.gripPose.position.x) < 0.32f);

    auto ApplyControllerPose = [](const RiddickVRTrackedPose& pose) {
        float qx = pose.orientation.x;
        float qy = pose.orientation.y;
        float qz = pose.orientation.z;
        float qw = pose.orientation.w;

        float r00 = 1.0f - 2.0f * (qy * qy + qz * qz);
        float r01 = 2.0f * (qx * qy - qz * qw);
        float r02 = 2.0f * (qx * qz + qy * qw);

        float r10 = 2.0f * (qx * qy + qz * qw);
        float r11 = 1.0f - 2.0f * (qx * qx + qz * qz);
        float r12 = 2.0f * (qy * qz - qx * qw);

        float r20 = 2.0f * (qx * qz - qy * qw);
        float r21 = 2.0f * (qy * qz + qx * qw);
        float r22 = 1.0f - 2.0f * (qx * qx + qy * qy);

        GLfloat m[16];
        m[0] = r00; m[1] = r10; m[2] = r20; m[3] = 0.0f;
        m[4] = r01; m[5] = r11; m[6] = r21; m[7] = 0.0f;
        m[8] = r02; m[9] = r12; m[10] = r22; m[11] = 0.0f;
        m[12] = pose.position.x;
        m[13] = pose.position.y;
        m[14] = pose.position.z;
        m[15] = 1.0f;

        s_gl.glMultMatrixf(m);
    };

    // 6. Render Left 3D Tactical Glove
    if (leftCtrl.gripPose.isValid) {
        s_gl.glPushMatrix();
        ApplyControllerPose(leftCtrl.gripPose);
        DrawTacticalHand(HandSide::Left, leftCtrl.grip, leftCtrl.trigger, isBoxerGuard, leftCtrl.buttons);
        s_gl.glPopMatrix();
    }

    // 7. Render Right 3D Tactical Glove
    if (rightCtrl.gripPose.isValid) {
        s_gl.glPushMatrix();
        ApplyControllerPose(rightCtrl.gripPose);
        DrawTacticalHand(HandSide::Right, rightCtrl.grip, rightCtrl.trigger, isBoxerGuard, rightCtrl.buttons);
        s_gl.glPopMatrix();
    }

    // 8. Restore matrices and OpenGL state
    s_gl.glMatrixMode(GL_MODELVIEW);
    s_gl.glPopMatrix();

    s_gl.glMatrixMode(GL_PROJECTION);
    s_gl.glPopMatrix();

    if (s_gl.glUseProgram && prevProgram != 0) {
        s_gl.glUseProgram(prevProgram);
    }

    s_gl.glPopAttrib();
}

void VRHandRenderer::DrawTacticalHand(HandSide side, float grip, float trigger, bool isBoxerGuard, uint32_t buttons) {
    s_gl.glPushMatrix();

    // Squeeze factor: grip trigger curls middle, ring, pinky
    float effectiveGrip = (std::max)(grip, isBoxerGuard ? 0.90f : 0.0f);
    float effectiveTrigger = (std::max)(trigger, isBoxerGuard ? 0.90f : 0.0f);

    // Capacitive sensing and trigger discipline (ADR 0411):
    // If boxer guard is active, both hands clench into complete fists
    // Otherwise, index finger respects capacitive touch & trigger pull (trigger discipline)
    bool triggerTouch = (buttons & RIDDICK_VR_TOUCH_TRIGGER) != 0 || trigger > 0.05f;
    bool thumbTouch = (buttons & (RIDDICK_VR_TOUCH_THUMBSTICK | RIDDICK_VR_TOUCH_THUMBREST |
                                 RIDDICK_VR_BTN_A | RIDDICK_VR_BTN_B | RIDDICK_VR_BTN_X | RIDDICK_VR_BTN_Y)) != 0;

    float indexCurl = 0.0f;
    if (isBoxerGuard) {
        indexCurl = 0.92f;
    } else if (effectiveGrip > 0.70f && trigger > 0.40f) {
        // Tight dual clench creates a full fist
        indexCurl = (std::max)(effectiveTrigger, effectiveGrip);
    } else if (triggerTouch) {
        // Finger resting on trigger guard or pulling trigger
        indexCurl = 0.15f + 0.80f * effectiveTrigger;
    } else {
        // Extended pointing / trigger discipline along barrel
        indexCurl = 0.0f;
    }

    float thumbCurl = 0.0f;
    if (isBoxerGuard) {
        thumbCurl = 0.88f;
    } else if (thumbTouch) {
        thumbCurl = 0.35f + 0.50f * effectiveGrip;
    } else {
        thumbCurl = 0.08f;
    }

    // 1. Forearm Gauntlet & Wrist Vambrace
    DrawForearmGauntlet(side);

    // 2. Palm & Armored Knuckle Guard
    DrawPalmAndKnuckleGuard(side);

    // 3. Articulated Digits (Fingers)
    // Splay angles in degrees: Index outward, Middle straight, Ring inward, Pinky inward
    float splayIndex  = (side == HandSide::Left) ? -4.5f :  4.5f;
    float splayMiddle = 0.0f;
    float splayRing   = (side == HandSide::Left) ?  3.0f : -3.0f;
    float splayPinky  = (side == HandSide::Left) ?  6.5f : -6.5f;

    DrawArticulatedFinger(side, 0, indexCurl,     splayIndex,  0.96f); // Index
    DrawArticulatedFinger(side, 1, effectiveGrip, splayMiddle, 1.00f); // Middle
    DrawArticulatedFinger(side, 2, effectiveGrip, splayRing,   0.93f); // Ring
    DrawArticulatedFinger(side, 3, effectiveGrip, splayPinky,  0.80f); // Pinky

    // 4. Articulated Thumb
    DrawArticulatedThumb(side, thumbCurl);

    s_gl.glPopMatrix();
}

void VRHandRenderer::DrawForearmGauntlet(HandSide side) {
    // Riddick's Prisoner Restraint Gauntlet / Vambrace
    // Tapered cylindrical sleeve extending from wrist (+0.04m) back along forearm (+0.20m)
    DrawTaperedCylinder(0.040f, 0.038f, 0.200f, 0.047f, 10, 0.12f, 0.12f, 0.14f);

    // Raised dorsal armor plate (+Y dorsal surface)
    DrawBeveledBox(-0.024f, 0.032f, 0.045f, 0.024f, 0.042f, 0.175f, 0.22f, 0.23f, 0.25f);

    // Metallic reinforcement rings & rivets
    DrawTaperedCylinder(0.048f, 0.039f, 0.058f, 0.040f, 10, 0.35f, 0.36f, 0.39f);
    DrawTaperedCylinder(0.120f, 0.042f, 0.130f, 0.043f, 10, 0.30f, 0.31f, 0.34f);

    // Left gauntlet: Beveled mounting bracket directly beneath the Diegetic Wrist Health HUD
    if (side == HandSide::Left) {
        DrawBeveledBox(-0.022f, 0.040f, 0.065f, 0.005f, 0.046f, 0.085f, 0.45f, 0.46f, 0.50f);
    }
}

void VRHandRenderer::DrawPalmAndKnuckleGuard(HandSide side) {
    // 1. Central Palm Body (Carpal/Metacarpal block)
    // Dark ballistic nylon / carbon fiber weave
    DrawBeveledBox(-0.034f, -0.016f, -0.042f, 0.034f, 0.020f, 0.038f, 0.13f, 0.13f, 0.15f);

    // 2. Palm Underside Ribbed Grip Pad (-Y palmar surface)
    DrawBeveledBox(-0.030f, -0.020f, -0.038f, 0.030f, -0.015f, 0.030f, 0.09f, 0.09f, 0.10f);

    // 3. Reinforced Carbon-Fiber Knuckle Protector (Boxer Knuckles)
    // Sits across metacarpal heads at z = [-0.048, -0.032] and +Y dorsal side
    DrawBeveledBox(-0.033f, 0.018f, -0.048f, 0.033f, 0.031f, -0.030f, 0.25f, 0.26f, 0.28f);

    // Knuckle studs / metallic striking ridges across each metacarpal
    for (int i = 0; i < 4; ++i) {
        float kx = -0.024f + i * 0.016f;
        DrawBeveledBox(kx - 0.006f, 0.028f, -0.050f, kx + 0.006f, 0.034f, -0.038f, 0.40f, 0.42f, 0.45f);
    }
}

void VRHandRenderer::DrawArticulatedFinger(HandSide side, int fingerIndex, float curl, float splayAngle, float lengthScale) {
    s_gl.glPushMatrix();

    // Knuckle root positions across distal palm (z = -0.045m)
    float baseOffsets[4] = { -0.024f, -0.008f, 0.008f, 0.024f };
    float xOffset = baseOffsets[fingerIndex];
    if (side == HandSide::Right) {
        xOffset = -xOffset;
    }

    // Move to knuckle joint
    s_gl.glTranslatef(xOffset, 0.005f, -0.045f);

    // Splay rotation (fan out across yaw axis)
    s_gl.glRotatef(splayAngle, 0.0f, 1.0f, 0.0f);

    // Dynamic curling angles:
    // When relaxed (curl ~ 0): slight natural curve (~6 deg)
    // When clenched (curl ~ 1): tight fist flexion
    float curlFactor = (std::clamp)(curl, 0.0f, 1.0f);
    float angleProx  = 6.0f + curlFactor * 62.0f;
    float angleInter = 8.0f + curlFactor * 78.0f;
    float angleDist  = 6.0f + curlFactor * 52.0f;

    float lProx  = 0.026f * lengthScale;
    float lInter = 0.020f * lengthScale;
    float lDist  = 0.015f * lengthScale;
    float w      = 0.0065f * lengthScale;

    // --- Segment 1: Proximal Phalanx ---
    s_gl.glRotatef(-angleProx, 1.0f, 0.0f, 0.0f); // Rotate downward/inward towards palm
    DrawBeveledBox(-w, -w, -lProx, w, w, 0.0f, 0.16f, 0.16f, 0.18f);

    // Segment 1 dorsal armor plate
    DrawBeveledBox(-w * 0.85f, w * 0.9f, -lProx * 0.9f, w * 0.85f, w * 1.3f, -lProx * 0.1f, 0.28f, 0.29f, 0.32f);

    s_gl.glTranslatef(0.0f, 0.0f, -lProx);

    // --- Segment 2: Intermediate Phalanx ---
    s_gl.glRotatef(-angleInter, 1.0f, 0.0f, 0.0f);
    DrawBeveledBox(-w * 0.9f, -w * 0.9f, -lInter, w * 0.9f, w * 0.9f, 0.0f, 0.15f, 0.15f, 0.17f);

    // Segment 2 joint knuckle ring
    DrawTaperedCylinder(-lInter * 0.15f, w * 0.95f, 0.0f, w * 0.95f, 6, 0.32f, 0.33f, 0.36f);

    s_gl.glTranslatef(0.0f, 0.0f, -lInter);

    // --- Segment 3: Distal Phalanx (Fingertip) ---
    s_gl.glRotatef(-angleDist, 1.0f, 0.0f, 0.0f);
    DrawBeveledBox(-w * 0.8f, -w * 0.8f, -lDist, w * 0.8f, w * 0.8f, 0.0f, 0.14f, 0.14f, 0.16f);

    // Fingertip tactility rubber grip pad
    DrawBeveledBox(-w * 0.7f, -w * 1.0f, -lDist * 0.9f, w * 0.7f, -w * 0.7f, -lDist * 0.1f, 0.08f, 0.08f, 0.09f);

    s_gl.glPopMatrix();
}

void VRHandRenderer::DrawArticulatedThumb(HandSide side, float curl) {
    s_gl.glPushMatrix();

    float xSign = (side == HandSide::Left) ? -1.0f : 1.0f;

    // Thumb originates from radial/palmar side
    s_gl.glTranslatef(xSign * 0.034f, -0.008f, 0.005f);

    // Thumb rest angle: angled forward and pitched inward
    s_gl.glRotatef(xSign * 28.0f, 0.0f, 1.0f, 0.0f);
    s_gl.glRotatef(-18.0f, 1.0f, 0.0f, 0.0f);

    float curlFactor = (std::clamp)(curl, 0.0f, 1.0f);
    float angleProx = 8.0f + curlFactor * 45.0f;
    float angleDist = 10.0f + curlFactor * 55.0f;

    float lProx = 0.024f;
    float lDist = 0.020f;
    float w = 0.0075f;

    // Proximal Thumb Segment
    s_gl.glRotatef(xSign * angleProx * 0.5f, 0.0f, 1.0f, 0.0f);
    s_gl.glRotatef(-angleProx, 1.0f, 0.0f, 0.0f);
    DrawBeveledBox(-w, -w, -lProx, w, w, 0.0f, 0.16f, 0.16f, 0.18f);

    s_gl.glTranslatef(0.0f, 0.0f, -lProx);

    // Distal Thumb Segment (folds over curled fingers when making a fist)
    s_gl.glRotatef(xSign * angleDist * 0.6f, 0.0f, 1.0f, 0.0f);
    s_gl.glRotatef(-angleDist, 1.0f, 0.0f, 0.0f);
    DrawBeveledBox(-w * 0.9f, -w * 0.9f, -lDist, w * 0.9f, w * 0.9f, 0.0f, 0.15f, 0.15f, 0.17f);

    // Thumb armor plate
    DrawBeveledBox(-w * 0.8f, w * 0.8f, -lDist * 0.85f, w * 0.8f, w * 1.2f, -lDist * 0.15f, 0.28f, 0.29f, 0.32f);

    s_gl.glPopMatrix();
}

void VRHandRenderer::DrawBeveledBox(float x0, float y0, float z0, float x1, float y1, float z1, float r, float g, float b) {
    s_gl.glColor3f(r, g, b);
    s_gl.glBegin(GL_QUADS);

    // Front (+Z)
    s_gl.glNormal3f(0.0f, 0.0f, 1.0f);
    s_gl.glVertex3f(x0, y0, z1);
    s_gl.glVertex3f(x1, y0, z1);
    s_gl.glVertex3f(x1, y1, z1);
    s_gl.glVertex3f(x0, y1, z1);

    // Back (-Z)
    s_gl.glNormal3f(0.0f, 0.0f, -1.0f);
    s_gl.glVertex3f(x1, y0, z0);
    s_gl.glVertex3f(x0, y0, z0);
    s_gl.glVertex3f(x0, y1, z0);
    s_gl.glVertex3f(x1, y1, z0);

    // Top (+Y)
    s_gl.glNormal3f(0.0f, 1.0f, 0.0f);
    s_gl.glVertex3f(x0, y1, z1);
    s_gl.glVertex3f(x1, y1, z1);
    s_gl.glVertex3f(x1, y1, z0);
    s_gl.glVertex3f(x0, y1, z0);

    // Bottom (-Y)
    s_gl.glNormal3f(0.0f, -1.0f, 0.0f);
    s_gl.glVertex3f(x0, y0, z0);
    s_gl.glVertex3f(x1, y0, z0);
    s_gl.glVertex3f(x1, y0, z1);
    s_gl.glVertex3f(x0, y0, z1);

    // Right (+X)
    s_gl.glNormal3f(1.0f, 0.0f, 0.0f);
    s_gl.glVertex3f(x1, y0, z1);
    s_gl.glVertex3f(x1, y0, z0);
    s_gl.glVertex3f(x1, y1, z0);
    s_gl.glVertex3f(x1, y1, z1);

    // Left (-X)
    s_gl.glNormal3f(-1.0f, 0.0f, 0.0f);
    s_gl.glVertex3f(x0, y0, z0);
    s_gl.glVertex3f(x0, y0, z1);
    s_gl.glVertex3f(x0, y1, z1);
    s_gl.glVertex3f(x0, y1, z0);

    s_gl.glEnd();
}

void VRHandRenderer::DrawTaperedCylinder(float z0, float r0, float z1, float r1, int slices, float r, float g, float b) {
    s_gl.glColor3f(r, g, b);
    s_gl.glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= slices; ++i) {
        float angle = i * (6.2831853f / slices);
        float ca = cosf(angle);
        float sa = sinf(angle);

        s_gl.glNormal3f(ca, sa, (r0 - r1) / (z1 - z0));
        s_gl.glVertex3f(ca * r0, sa * r0, z0);
        s_gl.glVertex3f(ca * r1, sa * r1, z1);
    }
    s_gl.glEnd();
}

} // namespace RiddickVR
