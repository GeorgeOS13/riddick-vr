# ADR 0031: Cybernetic Ghost Drone Piloting and Diegetic Targeting Reticle

## Status
Accepted

## Context
In *The Chronicles of Riddick: Assault on Dark Athena*, Riddick utilizes cybernetic uplink terminals to hijack automated Ghost Drones and patrol mechs:
1. **Remote Optics & Weaponry:** Riddick sees through the drone's optical feeds with night-vision visor overlays, piloting the drone through ventilation shafts and firing its mounted rapid-fire machine guns.
2. **VR Disconnect:** On desktop monitors, drone aiming and locomotion are locked to the mouse cursor. In Virtual Reality, players expect 6DOF motion controller weapon gimbal aiming independent of drone flight thrusters.
3. **Emergency Disconnect:** Players need an immediate, visceral way to sever the neural link and return to Riddick's physical body.

## Decision

We implement **Decoupled Drone Weapon Aiming & Dual-Stick Neural Ejection** in [`src/hook/camera_hook.cpp`](file:///C:/Users/GigelCastel/riddick-vr/src/hook/camera_hook.cpp):

```
                   [Drone Control Terminal Activated]
                                   │
                                   ▼
          ┌─────────────────────────────────────────────────┐
          │  Decoupled 6DOF Motion Aiming & Cyclic Fire     │
          │  - Right controller orientation directs turret  │
          │  - Right Trigger delivers 100ms cyclic pulses   │
          └─────────────────────────────────────────────────┘
                                   │
                                   ▼
                  [Emergency Ejection / Neural Sever]
          - Simultaneous Dual Stick Click: Left + Right
          - Sends 'E' + 'Escape' disconnect burst
          - Emits 0.85 static neural disconnect haptic wave
```

### 1. Dual-Thumbstick Emergency Neural Ejection
- Clicking both the Left Thumbstick and Right Thumbstick simultaneously (`stickLeft && stickRight`) issues an immediate disconnect burst (`VK_ESCAPE` and `'E'`), severing the uplink and returning the player to their physical body.
- Discharges a high-intensity $0.85$ amplitude neural disconnection static shockwave on both controllers.

### 2. Rapid-Fire Drone Gatling Feedback
- When firing during drone control, delivers high-frequency cyclic haptic pulses ($100\text{ ms}$, $0.65$ amplitude) matching the rapid-fire rotary rate of the drone's machine gun.

## Consequences

### Positive
- Intuitive, high-immersion cybernetic drone hijacking in Dark Athena.
- Emergency dual-stick ejection allows instant escape when Riddick's physical body is threatened by sneaking enemies.
