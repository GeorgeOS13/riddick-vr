#include "vr_hand_renderer.h"
#include "vr_config.h" // Track C #13: world scale
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

static RiddickVRVector3 HandWorldToHmdLocal(const RiddickVRVector3& worldPos, const RiddickVRTrackedPose& hmdPose) {
    float dx = worldPos.x;
    float dy = worldPos.y;
    float dz = worldPos.z;
    if (hmdPose.isValid && (fabsf(hmdPose.position.x) > 0.001f || fabsf(hmdPose.position.y) > 0.5f || fabsf(hmdPose.position.z) > 0.001f)) {
        dx -= hmdPose.position.x;
        dy -= hmdPose.position.y;
        dz -= hmdPose.position.z;
        float qx = -hmdPose.orientation.x;
        float qy = -hmdPose.orientation.y;
        float qz = -hmdPose.orientation.z;
        float qw = hmdPose.orientation.w;
        float norm = sqrtf(qx * qx + qy * qy + qz * qz + qw * qw);
        if (norm > 0.0001f) {
            qx /= norm; qy /= norm; qz /= norm; qw /= norm;
            float tx = 2.0f * (qy * dz - qz * dy);
            float ty = 2.0f * (qz * dx - qx * dz);
            float tz = 2.0f * (qx * dy - qy * dx);
            float rx = dx + qw * tx + (qy * tz - qz * ty);
            float ry = dy + qw * ty + (qz * tx - qx * tz);
            float rz = dz + qw * tz + (qx * ty - qy * tx);
            return { rx, ry, rz };
        }
    }
    return { dx, dy, dz };
}

static float HandDist(const RiddickVRVector3& a, const RiddickVRVector3& b) {
    float dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
    return sqrtf(dx * dx + dy * dy + dz * dz);
}

static GLDispatch s_gl;

VRHandRenderer& VRHandRenderer::Instance() {
    static VRHandRenderer s_instance;
    return s_instance;
}

void VRHandRenderer::SetHandFade(float leftFade, float rightFade) {
    m_leftFade = (std::clamp)(leftFade, 0.0f, 1.0f);
    m_rightFade = (std::clamp)(rightFade, 0.0f, 1.0f);
}

void VRHandRenderer::RenderHands(const RiddickVRSharedState& state, bool isLeftEye, uint32_t eyeWidth, uint32_t eyeHeight) {
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

    // GH #22: save every extra piece of state we mutate below (program,
    // buffer bindings, active matrix mode) so the game render state is
    // bit-identical after RenderHands returns.
    GLint prevMatrixMode = GL_MODELVIEW;
    if (s_gl.glGetIntegerv) {
        s_gl.glGetIntegerv(0x0BA0 /* GL_MATRIX_MODE */, &prevMatrixMode);
    }
    // Save previous GLSL program and unbind to allow clean fixed-function tactical glove rendering
    GLint prevProgram = 0;
    if (s_gl.glGetIntegerv) {
        s_gl.glGetIntegerv(0x8B8D /* GL_CURRENT_PROGRAM */, &prevProgram);
    }
    GLint prevArrayBuffer = 0;
    GLint prevElementArrayBuffer = 0;
    if (s_gl.glGetIntegerv) {
        s_gl.glGetIntegerv(0x8894 /* GL_ARRAY_BUFFER_BINDING */, &prevArrayBuffer);
        s_gl.glGetIntegerv(0x8895 /* GL_ELEMENT_ARRAY_BUFFER_BINDING */, &prevElementArrayBuffer);
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

    // 6. Render Left 3D Tactical Glove (with reload / NanoMED / heal interaction visuals)
    // HMD-local interaction volumes mirror camera_hook gesture zones so visuals match actions.
    RiddickVRVector3 leftLocal = HandWorldToHmdLocal(leftCtrl.gripPose.position, state.hmdPose);
    RiddickVRVector3 rightLocal = HandWorldToHmdLocal(rightCtrl.gripPose.position, state.hmdPose);
    float reloadDist = HandDist(leftLocal, rightLocal);
    bool nearReceiver = (reloadDist < 0.20f);
    auto InScanner = [](const RiddickVRVector3& p) {
        return (p.z >= -0.65f && p.z <= -0.20f) && (p.y >= -0.25f && p.y <= 0.42f) && (fabsf(p.x) < 0.30f);
    };
    auto PalmOpen = [](const RiddickVRControllerInput& c) {
        return (c.trigger < 0.20f && c.grip < 0.25f);
    };
    bool leftWaist = (leftLocal.y < -0.38f);
    bool rightWaist = (rightLocal.y < -0.38f);
    // Visual latches (renderer-local, no protocol change): waist grip implies cartridge in hand.
    bool leftHolding = leftWaist && (leftCtrl.grip > 0.60f);
    bool rightHolding = rightWaist && (rightCtrl.grip > 0.60f);
    bool leftHeal = PalmOpen(leftCtrl) && InScanner(leftLocal);
    bool rightHeal = PalmOpen(rightCtrl) && InScanner(rightLocal);
    float pulse = 0.5f + 0.5f * sinf(static_cast<float>(GetTickCount()) * 0.008f);
    float leftGlow = nearReceiver ? (0.4f + 0.6f * pulse) : 0.0f;
    float rightGlow = nearReceiver ? (0.4f + 0.6f * pulse) : 0.0f;
    // GH #46: fade our overlay where the engine's own gun/hands dominate the
    // view (firing grip, ADS pose, two-handed hold) so we don't double-render
    // over the game's arms. Full opacity for open/interacting hands.
    bool twoHanded = (leftCtrl.grip > 0.70f && rightCtrl.grip > 0.70f);
    float leftFade = (leftCtrl.trigger > 0.60f || twoHanded) ? 0.25f : 1.0f;
    float rightFade = (rightCtrl.trigger > 0.60f || twoHanded) ? 0.25f : 1.0f;
    VRHandRenderer::Instance().SetHandFade(leftFade, rightFade);
    // Grab proximity (vents, biometric terminals, guard throat-grip corridor):
    // hand forward of chest with grip closed (grab) or index extended (poke).
    auto GrabGlowFor = [](const RiddickVRVector3& local, const RiddickVRControllerInput& c) {
        bool inCorridor = (local.z >= -0.45f && local.z <= -0.15f) && (fabsf(local.x) < 0.30f);
        bool gripping = (c.grip > 0.50f);
        bool poking = (c.trigger < 0.20f && c.grip < 0.35f);
        if (!inCorridor || (!gripping && !poking)) return 0.0f;
        float depth = 1.0f - (local.z + 0.45f) / 0.30f; // 0 at entry -> 1 deep inside
        if (depth < 0.0f) depth = 0.0f;
        if (depth > 1.0f) depth = 1.0f;
        return 0.35f + 0.65f * depth;
    };
    float leftGrab = GrabGlowFor(leftLocal, leftCtrl);
    float rightGrab = GrabGlowFor(rightLocal, rightCtrl);
    if (leftCtrl.gripPose.isValid) {
        s_gl.glPushMatrix();
        ApplyControllerPose(leftCtrl.gripPose);
        DrawTacticalHand(HandSide::Left, leftCtrl.grip, leftCtrl.trigger, isBoxerGuard, leftCtrl.buttons,
                         leftGlow, leftHolding, leftHeal, leftGrab);
        s_gl.glPopMatrix();
    }

    // 7. Render Right 3D Tactical Glove
    if (rightCtrl.gripPose.isValid) {
        s_gl.glPushMatrix();
        ApplyControllerPose(rightCtrl.gripPose);
        DrawTacticalHand(HandSide::Right, rightCtrl.grip, rightCtrl.trigger, isBoxerGuard, rightCtrl.buttons,
                         rightGlow, rightHolding, rightHeal, rightGrab);
        s_gl.glPopMatrix();
    }

    // 8. Restore matrices and OpenGL state (GH #22: explicit restores for
    // everything RenderHands mutated; glPopAttrib alone does not cover
    // program/binding/matrix-mode state).
    s_gl.glMatrixMode(GL_MODELVIEW);
    s_gl.glPopMatrix();

    s_gl.glMatrixMode(GL_PROJECTION);
    s_gl.glPopMatrix();

    // Explicitly undo enable/disable changes (mirrors section 5 setup).
    s_gl.glDisable(GL_LIGHT0);
    s_gl.glDisable(GL_LIGHTING);
    s_gl.glDisable(GL_COLOR_MATERIAL);
    s_gl.glDisable(0x0BA7 /* GL_NORMALIZE */);
    s_gl.glDisable(GL_CULL_FACE);

    if (s_gl.glUseProgram) {
        s_gl.glUseProgram((GLuint)prevProgram);
    }
    if (s_gl.glBindBuffer) {
        s_gl.glBindBuffer(0x8892 /* GL_ARRAY_BUFFER */, (GLuint)prevArrayBuffer);
        s_gl.glBindBuffer(0x8893 /* GL_ELEMENT_ARRAY_BUFFER */, (GLuint)prevElementArrayBuffer);
    }
    s_gl.glMatrixMode(prevMatrixMode);

    s_gl.glPopAttrib();
}

void VRHandRenderer::DrawTacticalHand(HandSide side, float grip, float trigger, bool isBoxerGuard, uint32_t buttons,
                                      float interactionGlow, bool holdingCartridge, bool palmHeal, float grabGlow) {
    s_gl.glPushMatrix();

    // GH #46: select per-hand fade for this hand; all primitives multiply by it.
    m_currentFade = (side == HandSide::Left) ? m_leftFade : m_rightFade;

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

    // Track C #13: world scale multiplier on hand render scale (INI WorldScale).
    const float handScale = GetWorldScale();
    DrawArticulatedFinger(side, 0, indexCurl,     splayIndex,  0.96f * handScale); // Index
    DrawArticulatedFinger(side, 1, effectiveGrip, splayMiddle, 1.00f * handScale); // Middle
    DrawArticulatedFinger(side, 2, effectiveGrip, splayRing,   0.93f * handScale); // Ring
    DrawArticulatedFinger(side, 3, effectiveGrip, splayPinky,  0.80f * handScale); // Pinky

    // 4. Articulated Thumb
    DrawArticulatedThumb(side, thumbCurl);

    // 5. Interaction visuals: reload proximity glow, NanoMED cartridge, heal glow.
    // Fixed-function only, no new state leaks (restored via PopAttrib in RenderHands).
    // GH #46: warm diegetic palette - amber receiver, warm-orange NanoMED,
    // soft green scanner, warm-shifted cyan grab (all harmonized, fade-scaled).
    if (interactionGlow > 0.01f) {
        float g = interactionGlow;
        // Warm amber receiver-proximity highlight above knuckles.
        DrawBeveledBox(-0.023f, 0.024f, -0.052f, 0.023f, 0.030f, -0.028f,
                       0.30f + 0.60f * g, 0.24f + 0.38f * g, 0.16f + 0.06f * g);
    }
    if (holdingCartridge) {
        // Warm NanoMED cartridge block held in palm (waist draw visual).
        DrawBeveledBox(-0.014f, -0.009f, -0.030f, 0.014f, 0.009f, 0.010f,
                       0.82f, 0.48f, 0.14f);
        DrawBeveledBox(-0.014f, 0.009f, -0.030f, 0.014f, 0.014f, 0.010f,
                       0.95f, 0.68f, 0.22f);
    }
    if (palmHeal) {
        // Soft warm green scanner glow under palm.
        DrawBeveledBox(-0.021f, -0.017f, -0.036f, 0.021f, -0.011f, 0.028f,
                       0.18f, 0.72f, 0.34f);
    }
    if (grabGlow > 0.01f) {
        // Warm-shifted cyan grab highlight on fingertips region (object/person volumes).
        float g = grabGlow;
        DrawBeveledBox(-0.023f, -0.005f, -0.062f, 0.023f, 0.020f, -0.040f,
                       0.22f + 0.24f * g, 0.60f + 0.24f * g, 0.62f + 0.22f * g);
    }

    m_currentFade = 1.0f;
    s_gl.glPopMatrix();
}

void VRHandRenderer::DrawForearmGauntlet(HandSide side) {
    // GH #46: Butcher Bay prison-uniform sleeve cuff (slim, ~25% narrower).
    // Weathered grey-beige fabric sleeve, no armor - just cloth + wrist skin.
    DrawTaperedCylinder(0.040f, 0.0285f, 0.200f, 0.035f, 10, 0.46f, 0.44f, 0.38f);

    // Cuff fold: rolled sleeve hem near wrist (fabric ring, slightly darker).
    DrawTaperedCylinder(0.048f, 0.0295f, 0.060f, 0.0300f, 10, 0.40f, 0.38f, 0.32f);
    DrawTaperedCylinder(0.120f, 0.0315f, 0.130f, 0.0325f, 10, 0.42f, 0.40f, 0.34f);

    // Bare wrist skin between cuff and palm (weathered mid-brown-grey).
    DrawTaperedCylinder(0.020f, 0.0240f, 0.048f, 0.0260f, 10, 0.50f, 0.40f, 0.32f);

    // Left wrist: beveled mounting bracket directly beneath the Diegetic Wrist Health HUD (kept).
    if (side == HandSide::Left) {
        DrawBeveledBox(-0.017f, 0.030f, 0.065f, 0.004f, 0.035f, 0.085f, 0.45f, 0.46f, 0.50f);
    }
}

void VRHandRenderer::DrawPalmAndKnuckleGuard(HandSide side) {
    (void)side;
    // GH #46: slim bare prisoner hand (~25% narrower), weathered mid-brown-grey skin.
    // 1. Central palm body (carpal/metacarpal block) - skin base.
    DrawBeveledBox(-0.0255f, -0.012f, -0.042f, 0.0255f, 0.015f, 0.038f, 0.52f, 0.41f, 0.33f);

    // 2. Palm underside shading (-Y palmar surface, slightly deeper skin tone).
    DrawBeveledBox(-0.0225f, -0.015f, -0.038f, 0.0225f, -0.011f, 0.030f, 0.46f, 0.36f, 0.29f);

    // 3. Dorsal knuckle shading (subtle, skin-on-skin - no armor plate).
    DrawBeveledBox(-0.0248f, 0.0135f, -0.048f, 0.0248f, 0.0233f, -0.030f, 0.55f, 0.44f, 0.35f);

    // 4. Subtle knuckle wraps: cloth/leather wrap band across metacarpal heads
    // (replaces tactical studs; muted wrap tone).
    DrawBeveledBox(-0.0248f, 0.021f, -0.050f, 0.0248f, 0.0255f, -0.038f, 0.40f, 0.33f, 0.28f);
}

void VRHandRenderer::DrawArticulatedFinger(HandSide side, int fingerIndex, float curl, float splayAngle, float lengthScale) {
    s_gl.glPushMatrix();

    // Knuckle root positions across distal palm (z = -0.045m), slim ~25% spacing
    float baseOffsets[4] = { -0.018f, -0.006f, 0.006f, 0.018f };
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
    float w      = 0.0049f * lengthScale; // GH #46: slim fingers (~25% narrower)

    // --- Segment 1: Proximal Phalanx (bare skin) ---
    s_gl.glRotatef(-angleProx, 1.0f, 0.0f, 0.0f); // Rotate downward/inward towards palm
    DrawBeveledBox(-w, -w, -lProx, w, w, 0.0f, 0.52f, 0.41f, 0.33f);

    // Segment 1 dorsal shading (subtle skin highlight, no armor)
    DrawBeveledBox(-w * 0.85f, w * 0.9f, -lProx * 0.9f, w * 0.85f, w * 1.3f, -lProx * 0.1f, 0.55f, 0.44f, 0.35f);

    s_gl.glTranslatef(0.0f, 0.0f, -lProx);

    // --- Segment 2: Intermediate Phalanx ---
    s_gl.glRotatef(-angleInter, 1.0f, 0.0f, 0.0f);
    DrawBeveledBox(-w * 0.9f, -w * 0.9f, -lInter, w * 0.9f, w * 0.9f, 0.0f, 0.49f, 0.38f, 0.30f);

    // Segment 2 joint crease (skin crease shading, replaces knuckle ring)
    DrawTaperedCylinder(-lInter * 0.15f, w * 0.95f, 0.0f, w * 0.95f, 6, 0.38f, 0.29f, 0.23f);

    s_gl.glTranslatef(0.0f, 0.0f, -lInter);

    // --- Segment 3: Distal Phalanx (Fingertip) ---
    s_gl.glRotatef(-angleDist, 1.0f, 0.0f, 0.0f);
    DrawBeveledBox(-w * 0.8f, -w * 0.8f, -lDist, w * 0.8f, w * 0.8f, 0.0f, 0.46f, 0.36f, 0.29f);

    // Fingertip pad shading (deeper skin tone)
    DrawBeveledBox(-w * 0.7f, -w * 1.0f, -lDist * 0.9f, w * 0.7f, -w * 0.7f, -lDist * 0.1f, 0.40f, 0.31f, 0.25f);

    s_gl.glPopMatrix();
}

void VRHandRenderer::DrawArticulatedThumb(HandSide side, float curl) {
    s_gl.glPushMatrix();

    float xSign = (side == HandSide::Left) ? -1.0f : 1.0f;

    // Thumb originates from radial/palmar side (slim spacing)
    s_gl.glTranslatef(xSign * 0.026f, -0.008f, 0.005f);

    // Thumb rest angle: angled forward and pitched inward
    s_gl.glRotatef(xSign * 28.0f, 0.0f, 1.0f, 0.0f);
    s_gl.glRotatef(-18.0f, 1.0f, 0.0f, 0.0f);

    float curlFactor = (std::clamp)(curl, 0.0f, 1.0f);
    float angleProx = 8.0f + curlFactor * 45.0f;
    float angleDist = 10.0f + curlFactor * 55.0f;

    float lProx = 0.024f;
    float lDist = 0.020f;
    float w = 0.0056f; // GH #46: slim thumb (~25% narrower)

    // Proximal Thumb Segment (bare skin)
    s_gl.glRotatef(xSign * angleProx * 0.5f, 0.0f, 1.0f, 0.0f);
    s_gl.glRotatef(-angleProx, 1.0f, 0.0f, 0.0f);
    DrawBeveledBox(-w, -w, -lProx, w, w, 0.0f, 0.52f, 0.41f, 0.33f);

    s_gl.glTranslatef(0.0f, 0.0f, -lProx);

    // Distal Thumb Segment (folds over curled fingers when making a fist)
    s_gl.glRotatef(xSign * angleDist * 0.6f, 0.0f, 1.0f, 0.0f);
    s_gl.glRotatef(-angleDist, 1.0f, 0.0f, 0.0f);
    DrawBeveledBox(-w * 0.9f, -w * 0.9f, -lDist, w * 0.9f, w * 0.9f, 0.0f, 0.49f, 0.38f, 0.30f);

    // Thumb nail-side shading (subtle skin highlight, no armor)
    DrawBeveledBox(-w * 0.8f, w * 0.8f, -lDist * 0.85f, w * 0.8f, w * 1.2f, -lDist * 0.15f, 0.55f, 0.44f, 0.35f);

    s_gl.glPopMatrix();
}

void VRHandRenderer::DrawBeveledBox(float x0, float y0, float z0, float x1, float y1, float z1, float r, float g, float b) {
    // GH #46: darkening-path fade - multiply toward dark (no blending state touched).
    float f = (std::clamp)(m_currentFade, 0.0f, 1.0f);
    s_gl.glColor3f(r * f, g * f, b * f);
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
    float f = (std::clamp)(m_currentFade, 0.0f, 1.0f);
    s_gl.glColor3f(r * f, g * f, b * f);
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
