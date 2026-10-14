# Riddick VR: Escape from Butcher Bay

An open-source Virtual Reality (VR) mod for ***The Chronicles of Riddick: Escape from Butcher Bay*** (specifically the 2009 *Assault on Dark Athena* release containing the remastered Butcher Bay campaign), targeting modern 6DOF PC VR headsets via OpenXR.

Inspired by and building on architectural patterns from [F.E.A.R. VR](https://github.com/thefreemike31/fear-vr) (TheFreeMike & DR-89).

---

## 🐛 Bug Reports & Issue Tracking

All development progress, feedback, regressions, and bug reports are tracked through the **[GitHub Issues tab](../../issues)**. Please use the issue templates provided when reporting crashes, camera artifacts, or controller glitches.

---

## 🎯 Project Overview

*The Chronicles of Riddick* was years ahead of its time with its physical first-person body presence, brutal melee combat, dynamic stencil shadows, and stealth mechanics. Bringing this experience into PC VR requires:

1. **Native Stereo Rendering:** Injecting asymmetric projection matrices and IPD offsets into the Starbreeze Engine's OpenGL pipeline.
2. **First-Person Body Decoupling:** Isolating the 6DOF VR head tracking from the game's heavily animated neck and torso bone rigs to prevent motion sickness.
3. **Dual-Process 32-bit / 64-bit Architecture:** Bridging the 32-bit `DarkAthena.exe` executable to 64-bit OpenXR runtimes (Meta Quest Link, SteamVR, Virtual Desktop).
4. **Hardware Zero-Copy Texture Sharing:** Utilizing `WGL_NV_DX_interop` to share dual 2048x2048 eye buffers directly between 32-bit OpenGL and 64-bit Direct3D 11 swapchains without CPU readback stalls.
5. **Motion Controls & Melee:** Mapping fists, brass knuckles, shivs, Ulaks, and firearms to 6DOF tracked controllers with physical crouch detection, smooth/snap turning, and locomotion.
6. **Atmospheric FX Adaptation:** Adjusting Riddick's iconic "Eyeshine" night-vision mode, stencil shadow volumes, and minimalist diegetic HUD for stereoscopic depth convergence (~1.8m).

---

## 🏛️ Architecture Decision Records (ADRs)

Key technical choices, trade-offs, and implementation strategies are documented in the Architecture Decision Records:

| ADR | Title | Focus Area |
| :--- | :--- | :--- |
| [**ADR-0001**](docs/adr/0001-graphics-api-interception-and-opengl-hooking.md) | Graphics API Interception & OpenGL Hooking | Phase 0 & 1: OpenGL vs DX9 reality & `opengl32.dll` proxy |
| [**ADR-0002**](docs/adr/0002-dual-process-ipc-and-openxr-bridge.md) | Dual-Process IPC & 64-bit OpenXR Bridge | Phase 1: 32-bit game to 64-bit VR runtime communication |
| [**ADR-0003**](docs/adr/0003-stereoscopic-double-pass-rendering-and-shadow-volumes.md) | Stereoscopic Double-Pass Rendering & Shadows | Phase 1 & 3: Stereo eye injection and stencil shadow isolation |
| [**ADR-0004**](docs/adr/0004-camera-decoupling-and-first-person-body-awareness.md) | Camera Decoupling & First-Person Body Awareness | Phase 2: Head-bone detachment and comfort motion smoothing |
| [**ADR-0005**](docs/adr/0005-motion-controls-and-melee-combat-gestures.md) | Motion Controls & Melee Combat Translation | Phase 4: Gunplay and physical shiv/punch interactions |
| [**ADR-0006**](docs/adr/0006-diegetic-hud-eyeshine-and-post-processing.md) | Diegetic HUD, Eyeshine & Post-Processing Shaders | Phase 3: Night-vision shader adaptation and 3D floating HUD |
| [**ADR-0007**](docs/adr/0007-full-viewport-immersion-and-fov-projection-matching.md) | Full Viewport Immersion & FOV Projection Matching | Graphics: Elimination of action mode black bars and 1:1 human scale |
| [**ADR-0008**](docs/adr/0008-spatial-boxer-guard-and-gestural-melee-combat.md) | Spatial Boxer Guard & Gestural Melee Combat | Combat: Dual-fist guard pose detection and tactical reload gestures |
| [**ADR-0009**](docs/adr/0009-physical-ladder-climbing-and-mantling-gestures.md) | Physical Ladder Climbing & Mantling Gestures | Locomotion: 6DOF hand-over-hand climbing and dual-hand mantling |
| [**ADR-0010**](docs/adr/0010-diegetic-weapon-ammo-receiver-and-stealth-wall-peeking.md) | Diegetic Ammo Receiver HUD & Wall-Peeking | UI & Stealth: Slide backplate ammo counter and corner lean detection |
| [**ADR-0011**](docs/adr/0011-granular-melee-and-dual-blade-ulak-slashing-gestures.md) | Granular Melee & Dual-Blade Ulak Slashing | Combat: Scissor cross-slashes, shiv plunges, and throat slits |
| [**ADR-0012**](docs/adr/0012-spatial-eyeshine-and-tactical-shoulder-flashlight-gestures.md) | Spatial Eyeshine & Shoulder Flashlight Gestures | Controls: Right temple tap for Eyeshine and left shoulder flashlight tap |
| [**ADR-0013**](docs/adr/0013-tactile-waveform-synthesis-and-haptic-frequency-modulation.md) | Tactile Waveform Synthesis & Frequency Modulation | Haptics: Multi-band voice-coil synthesis (85 Hz to 280 Hz) |
| [**ADR-0014**](docs/adr/0014-spatial-weapon-recoil-and-tactical-foregrip-transmission.md) | Spatial Weapon Recoil & Foregrip Transmission | Gunplay: Weapon-isolated muzzle climb and off-hand haptic transmission |
| [**ADR-0015**](docs/adr/0015-spatial-weapon-holstering-and-inventory-gestures.md) | Spatial Weapon Holstering & Physical Inventory | Controls: Over-shoulder long-arm, right hip pistol, left hip Ulaks |
| [**ADR-0016**](docs/adr/0016-interactive-environmental-manipulation-vent-prying-and-retinal-biometrics.md) | Environmental Manipulation, Vents & Retinal Scanners | Interactions: Dual-hand vent grate prying and index finger terminal poke |
| [**ADR-0017**](docs/adr/0017-interactive-physical-throwing-and-object-propulsion.md) | Physical Object Throwing & Kinematic Propulsion | Physics: Off-hand grip swing-speed thresholded throwing vs quiet drops |
| [**ADR-0018**](docs/adr/0018-physical-melee-parrying-counter-strikes-and-disarms.md) | Physical Melee Parrying & Counter-Disarms | Combat: Guard-preserved forward thrust disarms and high-frequency shockwaves |
| [**ADR-0019**](docs/adr/0019-physical-medical-nanomed-station-activation-and-bio-restoration.md) | NanoMED Medical Station Activation & Bio-Restoration | Interactions: Flat open-palm contact healing and restorative haptic wave |
| [**ADR-0020**](docs/adr/0020-stealth-body-dragging-and-shadow-concealment-dynamics.md) | Stealth Body Dragging & Concealment Dynamics | Stealth: Low-frequency kinematic drag resistance when hauling corpses |
| [**ADR-0021**](docs/adr/0021-spatial-tactical-grenade-priming-and-pin-pulling.md) | Tactical Grenade Priming & Pin-Pulling | Weapons: Chest bandolier equip, two-handed pin-pulling, and throw release |
| [**ADR-0022**](docs/adr/0022-diegetic-eyeshine-anti-glare-and-photopic-adaptation.md) | Diegetic Eyeshine Photopic Adaptation & IPC Sync | Visuals & UI: 4-byte aligned state synchronization and gauntlet expansion |
| [**ADR-0023**](docs/adr/0023-heavy-scar-gun-and-turret-dual-hand-steering.md) | Heavy SCAR Gun Mine Detonation & Holstering | Gunplay: Low-spine holster draw and off-hand clench air mine detonation |

---

## 📁 Repository Layout

```
riddick-vr/
├── .github/
│   └── ISSUE_TEMPLATE/       # Templates for bug reports and feature requests
├── docs/
│   └── adr/                  # Architectural Decision Records (ADR 0001 - 0023)
├── include/
│   └── riddick_vr_protocol.h # IPC shared state, poses, matrix, controller formats
├── src/
│   ├── hook/                 # 32-bit opengl32.dll proxy, 368-symbol forwarders, stereo FBO, camera hook
│   ├── bridge/               # Low-latency shared memory IPC bridge & synchronization
│   └── host/                 # 64-bit OpenXR companion host process (riddickvr-host.exe)
├── tools/
│   ├── build-all.ps1         # Unified multi-architecture build script
│   ├── generate_forwarders.ps1 # Automated 368-symbol OpenGL forwarder generator
│   └── self_test_pipeline.py # Autonomous headless VR test runner
└── CMakeLists.txt            # Unified CMake build definition
```

---

## 🚀 Building & Prerequisites

* Windows 10 / 11 (x64)
* Visual Studio 2022 (with MSVC v143 toolsets for both Win32 and x64)
* CMake 3.25+
* Direct3D 11 & DXGI (included with standard Windows SDK)
* A legitimate copy of *The Chronicles of Riddick: Assault on Dark Athena* (GOG / retail PC release)

### Unified Multi-Arch Build
To build both the 64-bit Companion Host and 32-bit Game Proxy into `dist/`, run:
```powershell
.\tools\build-all.ps1
```

The script automatically produces:
* `dist\riddickvr-host.exe` (64-bit native OpenXR companion)
* `dist\opengl32.dll` (32-bit game proxy hook)

---

## 🎮 Installation & Play

1. Copy `dist\opengl32.dll` into your game executable directory:
   ```
   <GameFolder>\System\Win32_x86\opengl32.dll
   ```
2. Start your VR runtime (Virtual Desktop Streamer + VDXR, Meta Quest Link, or SteamVR).
3. (Optional) Run validation to check your OpenXR setup:
   ```powershell
   .\dist\riddickvr-host.exe --validate-only
   ```
4. Launch `riddickvr-host.exe` in the background or companion window.
5. Launch `DarkAthena.exe`! The game proxy will automatically link to the 64-bit host over shared memory and initialize zero-copy GPU texture sharing at full frame rate.

### Complete VR Gestures & Controls Mapping
* **Locomotion:** Left Thumbstick (`W`, `A`, `S`, `D`) with decoupled head look.
* **Turning:** Right Thumbstick X (Snap turning at $0.75$ deflection, smooth turning otherwise).
* **Dialogue / Menu Navigation:** Right Thumbstick Y (Up/Down arrow keys).
* **Physical Boxer Guard:** Raise both fists to chin level ($Y \in [-0.28, +0.18]\text{ m}, Z \in [-0.42, -0.12]\text{ m}$) with clenched grips ($> 0.40$) to engage solid block mode (`RMB`).
* **Melee Combos:** Fast physical jabs, hooks, uppercuts, and dual Ulak cross-slashes break guard instantly with zero latency.
* **Counter-Disarms & Parries:** Driving fists forward from Boxer Guard into an incoming strike triggers native Starbreeze disarms and bone-cracking shockwave haptics ($0.95$).
* **Physical Ladder Climbing:** Reach above shoulder and grip rungs to climb hand-over-hand; pull downward with both hands simultaneously to mantle up ledges.
* **Tactical Reloads:** Bring off-hand to weapon receiver ($< 20\text{ cm}$) or touch waist ammo pouch.
* **Two-Handed Firearm Stabilization:** Bring off-hand within $25\text{ cm}$ of firearm with grip held for SLERP aim stabilization and $0.65\times$ jitter dampening.
* **Tactical Inventory Holstering:**
  - *Over Right Shoulder:* Primary Long-Arm (`1`)
  - *Right Hip:* Sidearm / Pistol (`2`)
  - *Left Hip:* Dual Ulak Blades / Shivs (`3`)
  - *Chest Bandolier:* Tactical Grenades (`4`)
  - *Low Spine / Lumbar:* Heavy SCAR Rifle / Minigun (`5`)
* **Tactical Grenade Pin-Pull:** Bring off-hand to grenade ($< 16\text{ cm}$) and grip to pull pin; swing arm to launch.
* **SCAR Air Mine Detonation:** Squeeze off-hand trigger + grip clench in two-handed stance to detonate deployed mines.
* **Diegetic Wrist Health HUD:** Glance at top of left forearm gauntlet to inspect health blocks and stealth concealment status.
* **Diegetic Weapon Ammo Receiver:** Look at rear slide backplate of firearm to inspect live ammo counters.
* **Physical Crouching & Wall-Peeking:** Duck down $28\text{ cm}$ below standing eye-height to crouch; lean past corners with $15\text{ cm}$ deadband for tactile wall-peeking.
* **Environmental Manipulation:**
  - *Vent Grate Prying:* Reach both hands forward ($Z < -0.20\text{ m}$), grip, and wrench backward.
  - *Biometric Scanners:* Extend index finger and poke forward into scanner plane.
  - *NanoMED Health Stations:* Press flat open palm against dispenser pad for soothing bio-restoration.
* **Physical Object Throwing & Body Dragging:** Squeeze Left Grip to pick up objects/bodies; release during arm swing ($> 1.2\text{ m/s}$) to throw; haul while crouched for heavy drag friction resistance.
* **Tactical Eyeshine & Flashlight:** Tap right temple with grip to toggle Eyeshine (`Q`); tap left shoulder to toggle flashlight (`F`).

---

## 📜 License & Disclaimer

This project is an unofficial community modification. *The Chronicles of Riddick: Escape from Butcher Bay* and *Assault on Dark Athena* are the intellectual property of Starbreeze Studios, Atari, and Universal Studios. No copyrighted game assets or proprietary executables are included in this repository.
