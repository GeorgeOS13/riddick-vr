# ADR 0013: Tactile Waveform Synthesis and Haptic Frequency Modulation

## Status
Accepted

## Context
Standard VR controller haptics are frequently implemented as fixed-frequency, fixed-duration vibrations modulated only by linear amplitude. In *The Chronicles of Riddick: Assault on Dark Athena* and *Escape from Butcher Bay*, the physical sensations of combat and environmental interaction vary dramatically:
- Slicing through an enemy with curved Ulak blades or piercing a collarbone with a shiv is a sharp, high-frequency cutting event.
- Punching with bare knuckles or hoisting Riddick's body weight up a ladder lip is a low-frequency, heavy physical impact.
- Activating Eyeshine is a subtle, high-resonance optical buzz across the temple.
- Clicking the shoulder flashlight switch is a crisp mechanical snap.
- Subtle telemetry (such as wall-peeking past cover, ducking into physical crouch, or steadying a firearm two-handed) requires gentle tactile cues rather than distracting rumbles.

Modern OpenXR runtimes (Oculus Touch, Meta Quest 2/3 Touch Plus, and SteamVR) support full frequency modulation (in Hertz) and nanosecond duration control on linear resonant actuators and voice-coil transducers via `XrHapticVibration`.

## Decision

We implement a **Kinematic-to-Tactile Waveform Synthesis Engine** in `src/host/openxr_manager.cpp`:

```
                 [Hook Haptic Request: Pulse Amplitude]
                                   │
         ┌─────────────────────────┼─────────────────────────┐
         ▼                         ▼                         ▼
 [Lethal Blade Slices]   [Optical / Switch Res]    [Subtle Telemetry Cues]
 - Pulse >= 0.90         - Pulse 0.70 to 0.89      - Pulse < 0.45
 - Freq: 280 Hz          - Freq: 180 Hz            - Freq: 85 Hz
 - Dur: 60 ms            - Dur: 45 ms              - Dur: 35 ms
 - Sharp cutting buzz    - Snappy mechanical click - Deep tactile whisper
         │                         │                         │
         └─────────────────────────┼─────────────────────────┘
                                   ▼
                      [OpenXR Actuator Output]
                      xrApplyHapticFeedback()
```

### 1. Tactile Waveform Envelopes
The host compositor inspects incoming pulse amplitudes and maps them to tactile waveforms:

| Category | Gestures | Amplitude | Frequency | Duration | Tactile Texture |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Lethal Blade Slices** | Ulak cross-slashes, shiv plunge, throat-slit | $\ge 0.90$ | $280\text{ Hz}$ | $60\text{ ms}$ | Sharp, piercing high-frequency cut |
| **Resonant / Mechanical** | Eyeshine toggle, flashlight switch, gun fire | $0.70 - 0.89$ | $180\text{ Hz}$ | $45\text{ ms}$ | Snappy click / optical buzz |
| **Physical Impacts** | Mantle ledge hoist, magazine well slap | $0.45 - 0.69$ | $120\text{ Hz}$ | $75\text{ ms}$ | Heavy, solid mechanical thud |
| **Subtle Telemetry** | Corner wall-peek, rung grab, crouch, ADS steady | $< 0.45$ | $85\text{ Hz}$ | $35\text{ ms}$ | Deep, discreet tactile whisper |

### 2. Implementation in Host Compositor (`openxr_manager.cpp`)
```cpp
auto ApplyTactileHaptics = [&](XrPath handPath, float pulseVal) {
    if (pulseVal <= 0.05f || !xrApplyHapticFeedback) return;

    XrHapticVibration vib{XR_TYPE_HAPTIC_VIBRATION};
    vib.amplitude = (std::min)(pulseVal, 1.0f);

    if (pulseVal >= 0.90f) {
        vib.frequency = 280.0f; // 280 Hz sharp cutting edge
        vib.duration = 60000000; // 60ms
    } else if (pulseVal >= 0.70f) {
        vib.frequency = 180.0f; // 180 Hz snappy mechanical snap
        vib.duration = 45000000; // 45ms
    } else if (pulseVal >= 0.45f) {
        vib.frequency = 120.0f; // 120 Hz physical impact
        vib.duration = 75000000; // 75ms
    } else {
        vib.frequency = 85.0f;  // 85 Hz subtle tactile whisper
        vib.duration = 35000000; // 35ms
    }

    XrHapticActionInfo hapticInfo{XR_TYPE_HAPTIC_ACTION_INFO};
    hapticInfo.action = m_hapticAction;
    hapticInfo.subactionPath = handPath;
    xrApplyHapticFeedback(m_session, &hapticInfo, (XrHapticBaseHeader*)&vib);
};
```

## Consequences

### Positive
- Players receive instant tactile confirmation of their physical actions without visual distraction.
- Distinguishes slicing through flesh with blades from landing a blunt fist strike or mounting a ladder rung.
- Zero changes to the underlying IPC memory structure; immediately elevates tactile quality across all current and future systems.
