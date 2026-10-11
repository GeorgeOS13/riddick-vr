# Riddick VR: Escape from Butcher Bay

An open-source Virtual Reality (VR) mod for ***The Chronicles of Riddick: Escape from Butcher Bay*** (specifically the 2009 *Assault on Dark Athena* release containing the remastered Butcher Bay campaign), targeting modern 6DOF PC VR headsets via OpenXR.

Inspired by and building on architectural patterns from [F.E.A.R. VR](https://github.com/thefreemike31/fear-vr) (TheFreeMike & DR-89).

---

## ⚠️ Bug Reports & Issue Tracking

All development progress, feedback, regressions, and bug reports are tracked through the **[GitHub Issues tab](../../issues)**. Please use the issue templates provided when reporting crashes, camera artifacts, or controller glitches.

---

## 🎯 Project Overview

*The Chronicles of Riddick* was years ahead of its time with its physical first-person body presence, brutal melee combat, dynamic stencil shadows, and stealth mechanics. Bringing this experience into PC VR requires:

1. **Native Stereo Rendering:** Injecting asymmetric projection matrices and IPD offsets into the Starbreeze Engine's OpenGL pipeline.
2. **First-Person Body Decoupling:** Isolating the 6DOF VR head tracking from the game's heavily animated neck and torso bone rigs to prevent motion sickness.
3. **Dual-Process 32-bit / 64-bit Architecture:** Bridging the 32-bit `DarkAthena.exe` executable to 64-bit OpenXR runtimes (Meta Quest Link, SteamVR, Virtual Desktop).
4. **Motion Controls & Melee:** Mapping fists, brass knuckles, shivs, Ulaks, and firearms to 6DOF tracked controllers.
5. **Atmospheric FX Adaptation:** Adjusting Riddick's iconic "Eyeshine" night-vision mode, stencil shadow volumes, and minimalist diegetic HUD for stereo depth.

---

## 📚 Architecture Decision Records (ADRs)

Key technical choices, trade-offs, and implementation strategies are documented in the Architecture Decision Records:

| ADR | Title | Focus Area |
| :--- | :--- | :--- |
| [**ADR-0001**](docs/adr/0001-graphics-api-interception-and-opengl-hooking.md) | Graphics API Interception & OpenGL Hooking | Phase 0 & 1: OpenGL vs DX9 reality & `opengl32.dll` proxy |
| [**ADR-0002**](docs/adr/0002-dual-process-ipc-and-openxr-bridge.md) | Dual-Process IPC & 64-bit OpenXR Bridge | Phase 1: 32-bit game to 64-bit VR runtime communication |
| [**ADR-0003**](docs/adr/0003-stereoscopic-double-pass-rendering-and-shadow-volumes.md) | Stereoscopic Double-Pass Rendering & Shadows | Phase 1 & 3: Stereo eye injection and stencil shadow isolation |
| [**ADR-0004**](docs/adr/0004-camera-decoupling-and-first-person-body-awareness.md) | Camera Decoupling & First-Person Body Awareness | Phase 2: Head-bone detachment and comfort motion smoothing |
| [**ADR-0005**](docs/adr/0005-motion-controls-and-melee-combat-gestures.md) | Motion Controls & Melee Combat Translation | Phase 4: Gunplay and physical shiv/punch interactions |
| [**ADR-0006**](docs/adr/0006-diegetic-hud-eyeshine-and-post-processing.md) | Diegetic HUD, Eyeshine & Post-Processing Shaders | Phase 3: Night-vision shader adaptation and 3D floating HUD |

---

## 🏗️ Repository Layout

```
riddick-vr/
├── .github/
│   └── ISSUE_TEMPLATE/       # Templates for bug reports and feature requests
├── docs/
│   └── adr/                  # Architectural Decision Records
├── include/                  # Shared headers and interface definitions
├── src/
│   ├── hook/                 # 32-bit opengl32.dll proxy & memory injector
│   ├── bridge/               # Low-latency shared memory IPC & texture sharing
│   └── host/                 # 64-bit OpenXR companion process
└── CMakeLists.txt            # Build configuration
```

---

## 🚀 Building & Prerequisites

* Windows 10 / 11 (x64)
* Visual Studio 2022 (with MSVC v143 toolsets for both x86 and x64)
* CMake 3.25+
* OpenXR SDK (1.0+)
* A legitimate copy of *The Chronicles of Riddick: Assault on Dark Athena* (GOG / retail PC release)

---

## 📜 License & Disclaimer

This project is an unofficial community modification. *The Chronicles of Riddick: Escape from Butcher Bay* and *Assault on Dark Athena* are the intellectual property of Starbreeze Studios, Atari, and Universal Studios. No copyrighted game assets or proprietary executables are included in this repository.
