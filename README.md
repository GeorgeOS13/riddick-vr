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
| [**ADR-0024**](docs/adr/0024-heavy-mech-exoskeleton-and-sustained-rotary-fire.md) | Heavy Mech Exoskeleton & Sustained Rotary Fire | Combat & Haptics: Multi-band cyclic 90ms rotary recoil and foregrip transmission |
| [**ADR-0025**](docs/adr/0025-physical-crouch-jump-and-clearance-dynamics.md) | Physical Crouch-Jump & Clearance Dynamics | Movement: Coordinated knee-tuck clearance (+0.40m) into overhead vents |
| [**ADR-0026**](docs/adr/0026-physical-pipe-and-club-blunt-force-leverage.md) | Two-Handed Heavy Pipe & Blunt Overhead Slams | Combat: Simultaneous downward arm swing (Vy < -1.30 m/s) concussive slam |
| [**ADR-0027**](docs/adr/0027-tactical-off-hand-flashlight-harries-technique.md) | Tactical Flashlight & Harries Shooting Stance | Gunplay: Crossed-wrist stabilization (alpha = 0.22) in pitch-black corridors |
| [**ADR-0028**](docs/adr/0028-cryo-hibernation-capsule-emergency-release.md) | Cryo-Pod Emergency Canopy Seal Ejection | Narrative: Dual-palm forward shove (Vz < -0.28 m/s) decompression breakout |
| [**ADR-0029**](docs/adr/0029-diegetic-collectible-inspection-cigarette-packs-and-bounty-cards.md) | Diegetic Collectible Inspection: Cigarette Packs & Cards | Interactions: Off-hand eye-level inspection (Y > -0.05m, Z > -0.32m) with 180Hz buzz |
| [**ADR-0030**](docs/adr/0030-laser-tripwire-dynamic-crouch-limbo-evasion.md) | Laser Tripwire Limbo Evasion & Prone Crawl Pacing | Stealth: Head drop > 48cm deep prone posture with 600ms crawl heartbeat |
| [**ADR-0031**](docs/adr/0031-cybernetic-ghost-drone-piloting-and-diegetic-targeting-reticle.md) | Cybernetic Ghost Drone Piloting & Neural Ejection | Systems: Dual-thumbstick emergency disconnect burst and decoupled drone aiming |
| [**ADR-0032**](docs/adr/0032-interactive-guard-interrogation-and-physical-throat-grip-shakedown.md) | Interactive Guard Interrogation & Throat-Grip Shakedown | Interactions: Forward collar reach, throat grip, and forward/back shake intimidation |
| [**ADR-0033**](docs/adr/0033-vent-duct-pipe-hang-and-aerial-drop-takedowns.md) | Vent Duct Pipe Hang & Death-From-Above Aerial Drops | Combat: Steep downward look (pitch < -40 deg) with downward plunge (Vy < -1.1 m/s) |
| [**ADR-0034**](docs/adr/0034-riot-shotgun-physical-pump-action-and-chambering.md) | Riot Shotgun Physical Pump Action & Chambering | Gunplay: Two-handed slide rack rearward (0.55 click) and forward battery lock (0.75) |
| [**ADR-0035**](docs/adr/0035-physical-shiv-sharpening-and-blade-grinding.md) | Physical Shiv Sharpening & Blade Grinding | Combat: Close edge reciprocating strokes (|Delta Y| > 8cm) with 220Hz metallic scrape |
| [**ADR-0036**](docs/adr/0036-ventilation-duct-screw-fastener-unthreading.md) | Vent Duct Screw Fastener Unthreading | Interactions: Grate corner contact and wrist twist ratchet unthreading (160Hz click) |
| [**ADR-0037**](docs/adr/0037-mining-core-pneumatic-drill-and-seismic-haptics.md) | Mining Core Pneumatic Rock Drill Seismic Haptics | Tools & Haptics: Dual-trigger industrial hammering with cyclic 65Hz seismic shockwaves |
| [**ADR-0038**](docs/adr/0038-contraband-smuggling-and-ankle-pouch-inventory.md) | Contraband Smuggling & Ankle Pouch Inventory | Inventory & Stealth: Crouched reaching down to boot (Y < -0.65m) to stash/retrieve contraband |
| [**ADR-0039**](docs/adr/0039-security-tranq-gun-tactile-charging-and-stun-darts.md) | Security Tranq Gun Tactile Charging & Stun Darts | Weapons: Escalating electrostatic hum (80Hz to 260Hz) and 0.85 electrical discharge snap |
| [**ADR-0040**](docs/adr/0040-dark-athena-eva-spacewalk-magnetic-boots-and-airlocks.md) | Dark Athena EVA Spacewalk Magnetic Boots & Airlocks | Locomotion: Alternating magnetic sole clamp thuds (75Hz) across exterior ship hulls |
| [**ADR-0041**](docs/adr/0041-pit-fighting-arena-ring-ropes-and-concussive-shoves.md) | Pit Fighting Arena Ring Ropes & Concussive Shoves | Combat: Dual-palm forward thrust concussive shove with 0.85 spring boundary recoil |
| [**ADR-0042**](docs/adr/0042-sniper-rifle-diegetic-eye-relief-and-bipod-bracing.md) | Sniper Rifle Diegetic Eye Relief & Bipod Bracing | Gunplay: Close eye relief (< 16cm) aim dampening (alpha = 0.12) and bipod lock (alpha = 0.05) |
| [**ADR-0043**](docs/adr/0043-tactical-circuit-breaker-sabotage-and-power-box-lever.md) | Circuit Breaker Sabotage & Power Box Lever | Stealth: Wall breaker contact and downward pull (Vy < -0.40m/s) with 0.80 contactor clack |
| [**ADR-0044**](docs/adr/0044-manual-bulkhead-handwheel-spindle-cranking.md) | Manual Bulkhead Handwheel Spindle Cranking | Interactions: Circular tangential hand revolutions with 160Hz dogging ratchet clicks (0.45) |
| [**ADR-0045**](docs/adr/0045-inmate-gambling-dice-shaking-and-roll-throwing.md) | Inmate Gambling Dice Shaking & Roll Throwing | Mini-Games: Cupped hand reciprocating shake rattle (160Hz, 0.30) and roll throw release |
| [**ADR-0046**](docs/adr/0046-atmospheric-hull-rupture-and-emergency-blast-shield.md) | Decompression Rupture & Emergency Blast Shield | Narrative: Dual-hand overhead latch (Y > 0.25m) and downward pull (Vy < -0.5m/s) 0.95 slam |
| [**ADR-0047**](docs/adr/0047-guard-subdue-chokehold-and-silent-neck-snap.md) | Guard Subdue Chokehold & Silent Neck Snap | Stealth: Rear chest chokehold (100Hz struggle) with lateral twist cervical snap (280Hz, 0.95) |
| [**ADR-0048**](docs/adr/0048-mining-cart-handbrake-and-rail-shunting.md) | Mining Cart Handbrake & Rail Shunting | Locomotion: Lever push/pull throttle & braking with 60Hz steel wheel rail shudder (0.40) |
| [**ADR-0049**](docs/adr/0049-heavy-mech-cockpit-canopy-and-seal-purge.md) | Heavy Mech Cockpit Canopy & Emergency Seal Purge | Vehicles: Overhead canopy slam down (0.85 surge) and dual-grip/stick emergency ejection |
| [**ADR-0050**](docs/adr/0050-inmate-bunk-mattress-stash-search-and-shiv-concealment.md) | Inmate Bunk Mattress Stash Search & Contraband | Exploration: Crouched off-hand mattress lift with dominant under-bed sweep (200Hz chime) |

---

## 📁 Repository Layout

```
riddick-vr/
├── .github/
│   └── ISSUE_TEMPLATE/       # Templates for bug reports and feature requests
├── docs/
│   └── adr/                  # Architectural Decision Records (ADR 0001 - 0050)
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
* **Diegetic Collectible Inspection:** Bring off-hand holding cigarette packs or bounty cards up to eye level ($Y > -0.05\text{ m}, Z > -0.32\text{ m}$) with Left Grip to inspect fine detail with $180\text{ Hz}$ tactile buzz.
* **Laser Tripwire Deep Prone Limbo:** Duck lower than $48\text{ cm}$ below standing eye-height to drop into deep prone crawl, slipping beneath laser grids with rhythmic $600\text{ ms}$ stealth crawl pulses.
* **Cybernetic Drone Uplink & Ejection:** Steer drones with motion aim; click both thumbsticks simultaneously to sever link with high-intensity static surge ($0.85$).
* **Guard Interrogation & Throat Shakedown:** Reach forward to grab guards/snitches by the throat with Left Grip; jerk forward/back ($|V_z| > 0.7\text{ m/s}$) to intimidate and advance dialogue.
* **Death-From-Above Aerial Takedowns:** Hang over ledges or drop from overhead ducts looking down ($\text{pitch} < -40^\circ$); plunge downward with weapons to execute instant aerial takedowns with $0.95$ ground impact shockwaves.
* **Riot Shotgun Manual Pump Action:** In two-handed stance, rack the off-hand forend rearward ($> 6\text{ cm}$) to eject spent shells ($0.55$), then slam forward into battery ($0.75$) to chamber buckshot.
* **Physical Shiv Sharpening:** Rub off-hand whetstone or blade along dominant shiv ($dist < 15\text{ cm}$) in reciprocal strokes ($|\Delta V_y| > 0.55\text{ m/s}$) for $220\text{ Hz}$ edge-honing rasp haptics.
* **Vent Fastener Unthreading:** Contact vent corners with tool/shiv and twist wrist counter-clockwise ($|V_x| > 0.45\text{ m/s}$) to pop corner screws with $160\text{ Hz}$ ratchet clicks.
* **Mining Core Pneumatic Drill:** Hold both triggers in two-handed stance to unleash heavy industrial rock-shattering percussion with cyclic $65\text{ Hz}$ seismic vibrations ($0.95$).
* **Contraband Ankle Boot Holster:** Reach down to ankle boot ($Y < -0.65\text{ m}$) while crouched or prone to access concealed contraband pouch with $140\text{ Hz}$ fabric rustle.
* **Security Tranq Gun Capacitor Charge:** In aiming stance, hold trigger partially down ($0.35 - 0.90$) to build electrostatic voltage ($80\text{ Hz} \to 260\text{ Hz}$); release to fire high-voltage stun dart with $0.85$ discharge snap.
* **EVA Spacewalk Magnetic Boots:** Moving across exterior hull surfaces generates alternating magnetic sole clamping thuds ($0.35$ at $75\text{ Hz}$) locking Riddick to the deck in deep space.
* **Pit Fighting Arena Concussive Shove:** Thrust both fists/palms forward simultaneously ($V_z < -1.10\text{ m/s}$) to stagger opponents with $0.85$ spring boundary shockwaves.
* **Sniper Rifle Diegetic Eye Relief & Bipod:** Bring scope close to eye ($< 16\text{ cm}$) for precision dampening ($\alpha = 0.12$); rest off-hand steady against railings for rock-solid bipod lock ($\alpha = 0.05$).
* **Circuit Breaker Lever Sabotage:** Grip wall breaker lever ($Z \in [-0.45, -0.20]\text{ m}$) and yank downward ($V_y < -0.40\text{ m/s}$) to kill sector power with $0.80$ mechanical clack.
* **Bulkhead Handwheel Spindle:** Grip manual door wheel and crank in circular tangential revolutions ($|V| > 0.50\text{ m/s}$) for $160\text{ Hz}$ dogging ratchet clicks ($0.45$).
* **Inmate Gambling Dice Mini-Games:** Cup hand with grip held ($0.40 - 0.85$) and shake ($> 0.70\text{ m/s}$) for $160\text{ Hz}$ rattling bone haptics ($0.30$); release forward to roll.
* **Emergency Blast Shield Slam:** Raise both hands overhead ($Y > 0.25\text{ m}$) at bulkhead station and slam down hard ($V_y < -0.50\text{ m/s}$) for dual $0.95$ impact slam.
* **Guard Subdue Chokehold & Silent Neck Snap:** Sneak in crouch/prone with left arm across guard neck ($Z \in [-0.35, -0.15]\text{ m}, Y \in [-0.20, 0.10]\text{ m}$) with Left Grip held ($100\text{ Hz}$ struggle); jerk dominant hand laterally ($|V_x| > 0.80\text{ m/s}$) for $280\text{ Hz}$ ($0.95$) silent cervical neck snap.
* **Sub-Level Mining Cart Handbrake & Throttle:** Grip cart brake lever ($Z \in [-0.35, -0.15]\text{ m}$) and push/pull ($|V_z| > 0.35\text{ m/s}$) to toggle throttle/handbrake (`W`/`S`) with $60\text{ Hz}$ rail shudder ($0.40$).
* **Heavy Mech Cockpit Canopy Slam & Purge:** Reach overhead in mech cockpit ($Y > 0.20\text{ m}$) and slam down ($V_y < -0.45\text{ m/s}$) to seal hermetic armor canopy; click dual thumbsticks with both grips held to blow canopy bolts ($0.95$).
* **Inmate Bunk Mattress Stash Search:** Crouch and lift bunk mattress with off-hand while sweeping dominant hand under bed frame ($|V_x| > 0.35\text{ m/s}$) to discover hidden shivs and contraband with $200\text{ Hz}$ ($0.60$) discovery chime.
* **Cryo-Pod Cryo-Glass Condensation Wipe:** Sweep open palm/gauntlet across frosted observation glass ($|V| > 0.40\text{ m/s}$) for $240\text{ Hz}$ ($0.35$) squeaking glass friction micro-haptics and progressive mist clearing.
* **Dual-Hand Riot Shield Plant & Bash:** In two-handed stance, plunge controllers downward ($V_y < -0.75\text{ m/s}$) to plant shield into deck grating (`C` + `Space`, $80\text{ Hz}$ clamp); thrust both hands forward ($V_z < -1.15\text{ m/s}$) for concussive Shield Bash (`LMB`, $0.95$ shockwave).
* **Subterranean Steam Pipe Wheel Valve:** Grip industrial wheel valve ($Z \in [-0.42, -0.18]\text{ m}$) and crank 3 revolutions ($|V| > 0.45\text{ m/s}$) to vent high-pressure scalding steam (`E` + `Q`) with sustained $110\text{ Hz}$ ($0.70$) pneumatic hissing.
* **Inmate Pit Brawl Stool Wrench & Smash:** Crouch near bolted metal furniture, grip with both hands ($> 0.60$), and yank upward violently ($V_y > 1.05\text{ m/s}$) to rip stool from deck rivets (`E` + Slot '1', $0.85$ crack); two-handed swings ($|V| > 1.35\text{ m/s}$) deliver bone-crushing blunt smashes (`LMB`, dual $0.95$).
* **Tactical Riot Baton Electric Arc Shock:** Squeeze dominant trigger ($> 0.60$) during forward thrust ($V_z < -0.90\text{ m/s}$) to channel high-voltage capacitor current with $260\text{ Hz}$ ($0.90$) electrical sizzle shockwave.
* **Inmate Shiv Boot Sheath Draw:** Reach down to inside calf/ankle ($Y < -0.60\text{ m}, X \in [-0.25, 0.0]\text{ m}$) while crouched or prone and grip to draw/holster concealed shiv (Slot '3') with $160\text{ Hz}$ ($0.45$) sheath friction click.
* **Cargo Crane Hydraulic Console Steering:** Grasp gantry crane levers ($Z \in [-0.40, -0.15]\text{ m}, Y \in [-0.35, -0.05]\text{ m}$) with grip held; push/pull and throw laterally to drive boom/trolley (`W`/`S`/`A`/`D`) with $50\text{ Hz}$ ($0.35$) hydraulic rumble.
* **Security Drone EMP Overload & Chassis Snap:** Grapple stunned security drone collar ($Z \in [-0.45, -0.20]\text{ m}$) with both grips clenched ($> 0.65$) and twist opposing hands violently ($|\Delta V_y| > 1.20\text{ m/s}$) to snap cybernetic spine (`LMB` + `'E'`) with dual $0.95$ ($280\text{ Hz}$) EMP arc burst.
* **Laser Tripwire Shield Deflection:** Hold riot shield forward in two-handed stance ($Z \in [-0.35, -0.15]\text{ m}$) to deflect sweeping infrared beams without triggering sector alarms ($220\text{ Hz}$ scatter haptics).
* **Inmate Mess Hall Tray Parry & Batter:** Grip mess tray from cafeteria table ($Y \in [-0.55, -0.25]\text{ m}$) with Left Grip (`E` + Slot '1'); thrust forward ($V_z < -1.0\text{ m/s}$) for facial tray smash (`LMB`, $0.90$).
* **Hydraulic Pressure Blast Valve Steam Jet:** Strike/kick wall blast valve ($Z \in [-0.40, -0.18]\text{ m}, V_z < -1.10\text{ m/s}$) to pop emergency relief seal (`E` + `'Q'`), unleashing scalding steam jets with $130\text{ Hz}$ ($0.75$) rumble.
* **Cybernetic Drone Reticle Lead & Decoupled Aim:** Drone cockpit view tracks natural HMD look while heavy servo-driven turret tracks motion aim with dynamic lag filtering ($\alpha = 0.28$) and lock-on confirmation.
* **Guard Rifle Stripping & Mag Ejection Disarm:** While holding Boxer Guard, squeeze both triggers ($> 0.60$) and thrust forward ($V_z < -1.15\text{ m/s}$) to violently wrench an assailant's rifle away, ejecting their ammo magazine (`R`), and claiming the firearm (`1`) with $280\text{ Hz}$ ($0.95$) receiver shearing haptics.
* **Bio-Containment Decontamination Airlock Lever:** Grip wall-mounted chamber valve handle ($Z \in [-0.42, -0.18]\text{ m}, Y \in [0.05, 0.35]\text{ m}$) and pull downward ($V_y < -0.60\text{ m/s}$) to cycle quarantine pressure seals (`E`) with $140\text{ Hz}$ ($0.80$) pressurized whoosh haptics.
* **Inmate Pit Brawl Dirt & Ash Throw Blind:** While crouched or in deep prone, flick open dominant hand upward and forward ($V_y > 0.80\text{ m/s}, V_z < -0.80\text{ m/s}$) without gripping to scoop floor soot/grit and blind adversaries (`E` + `LMB`) with $180\text{ Hz}$ ($0.50$) particulate scatter micro-haptics.
* **Heavy Exoskeleton Power Fist Kinetic Shockwave Punch:** Clench dominant grip ($> 0.60$) and drive arm forward at high velocity ($V_z < -1.60\text{ m/s}$) to trigger hydraulic piston discharge (`W` + `LMB`), detonating a concussive shockwave ($0.98$ concussive blast at $100\text{ Hz}$) that obliterates armor.
* **Overhead Vent Hatch Dual-Palm Shove:** Look upward ($\text{pitch} > +26^\circ$) and shove both open palms upward ($Y > 0.25\text{ m}, V_y > 0.85\text{ m/s}$) to pop ceiling conduit access latches (`E` + `Space`) with $160\text{ Hz}$ ($0.80$) resonant spring latch haptics.
* **Inmate Toothbrush Shank Whittling:** Crouch or drop prone, clutch improvised shank near the floor ($Y < -0.45\text{ m}$), and perform rapid lateral friction strokes ($|V_x| > 0.65\text{ m/s}$) against concrete masonry to grind a razor tip (`E` + `3`) with $200\text{ Hz}$ abrasive rasp haptics.
* **Emergency Oxygen Mask Quick-Don:** Bring off-hand controller to chin/mouth zone ($|X| < 0.12\text{ m}, Y \in [-0.18, 0.06]\text{ m}, Z \in [-0.20, -0.05]\text{ m}$) with grip held for $150\text{ ms}$ to clamp rebreather to face (`E`) with $110\text{ Hz}$ ($0.65$) pneumatic intake seal haptics.
* **Automated Turret Slew & Spade Grips:** Grasp dual turret handles ($Z \in [-0.42, -0.16]\text{ m}$) with both grips clenched; steer azimuth via forward/back hand offset ($\Delta Z > \pm 8\text{ cm}$) and pitch via elevation ($\Delta Y > \pm 12\text{ cm}$) with deep $55\text{ Hz}$ ($0.45$) hydraulic gear train rumble.
* **Radio Transceiver Intercept & Eavesdropping:** Bring either controller to ear/temple ($|X| > 0.16\text{ m}, Y \in [0.05, 0.22]\text{ m}$) with grip held for $120\text{ ms}$ to tap into intercepted guard radio chatter (`Tab`) with $220\text{ Hz}$ ($0.40$) squelch burst haptics.
* **Cargo Container Hydraulic Latch Lever:** Grasp container lock handle ($Z \in [-0.42, -0.16]\text{ m}, Y \in [-0.25, 0.15]\text{ m}$) with grip held and throw upward/outward ($|V| > 0.70\text{ m/s}$) to pop pressurized cargo doors (`E`) with $100\text{ Hz}$ ($0.85$) hydraulic decompression thud.
* **Corneal Blink & Eye Recovery Wipe:** When blinded by dust or flashbangs, sweep forearm across eyes ($Z \in [-0.22, -0.05]\text{ m}$) or rapidly shake head ($|\Delta\text{yaw}| > 0.15\text{ rad}$) to accelerate visual recovery (`Space`) with $180\text{ Hz}$ ($0.30$) corneal blink micro-haptics.
* **Subterranean Cryo-Nitrogen Coolant Valve:** Strike or wrench frost-encrusted pipe valve ($Z \in [-0.42, -0.16]\text{ m}, |V| > 1.05\text{ m/s}$) to crack ice seals (`E` + `Q`), venting supercooled cryogenic clouds with $260\text{ Hz}$ ($0.85$) crystalline shatter haptics.
* **Inmate Gambling Card Dealing & Flipping:** Above table level ($Y \in [-0.45, -0.10]\text{ m}$), pinch cards ($Grip \in [0.40, 0.75]$) and flick wrist laterally ($|V_x| > 0.60\text{ m/s}$) to deal cards (`E` + `Enter`) with $240\text{ Hz}$ ($0.35$) card slide micro-haptics.
* **Mining Rail Track Switch Lever:** Grasp low track switch handle ($Y \in [-0.55, -0.15]\text{ m}, Z \in [-0.42, -0.16]\text{ m}$) with grip held and swing laterally across tracks ($|V_x| > 0.70\text{ m/s}$) to divert rail routes (`E`) with $70\text{ Hz}$ ($0.85$) iron rail clank.
* **Two-Handed Stun Baton Defibrillator Slam:** In two-handed stance, hold both triggers ($> 0.60$) and plunge controllers downward violently ($V_y < -1.30\text{ m/s}$) to detonate an explosive electro-kinetic ground shockwave (`S` + `LMB`, dual $0.98$ blast at $280\text{ Hz}$).
* **EVA Airlock Dogging Spindle Cranking:** Grasp exterior airlock wheel ($Z \in [-0.42, -0.16]\text{ m}$) with both grips held and crank 3 full circular revolutions ($|V| > 0.55\text{ m/s}$) to dog vacuum seals (`E`) with $100\text{ Hz}$ ($0.85$) hermetic clamp thud.
* **Atmospheric Pressure Equalization Petcock Valve:** Grasp needle valve spindle on door frame ($Z \in [-0.40, -0.15]\text{ m}, Y \in [-0.10, 0.20]\text{ m}$) with grip held and twist wrist ($|V| > 0.45\text{ m/s}$) to equalize pressure (`E`) with $220\text{ Hz}$ ($0.55$) high-frequency hissing haptics.
* **Inmate Contraband Spork Whittling:** While crouched near cafeteria tables/benches ($Y \in [-0.55, -0.25]\text{ m}$), perform 4 reciprocating friction strokes ($|V_x| > 0.60\text{ m/s}$) to notch utensil tines into a triple-point puncture shiv (`E` + `3`) with $190\text{ Hz}$ ($0.45$) serrated rasp haptics.
* **Security Turret Optical Sensor Spray Paint Blind:** Reach up to sensor dome ($Y \in [0.10, 0.40]\text{ m}, Z \in [-0.35, -0.15]\text{ m}$) with grip held and sweep laterally ($|V_x| > 0.50\text{ m/s}$) to coat optics with spray paint/soot (`E` + `Q`), disabling tracking with $160\text{ Hz}$ ($0.50$) propellant hiss.
* **Drone Battery Power Cell Hot-Swap Ejection:** Bring off-hand close to weapon receiver ($dist < 18\text{ cm}$) with grip held and flick downward ($V_y < -0.70\text{ m/s}$) to eject spent thermal power cell (`R`) with $140\text{ Hz}$ ($0.75$) magnetic decoupling snap.
* **DNA Biometric Weapon Grip Shock Suppression:** Bring off-hand to weapon grip ($dist < 15\text{ cm}$) with Left Grip held ($> 0.60$) to wrap insulating gauntlet fabric around the handle (`1`), suppressing biometric shock discharge and enabling two-handed stabilization with $130\text{ Hz}$ ($0.45$) cloth wrap dampening.
* **Ventilation Axial Fan Blade Jamming:** While crouched near industrial rotor hub ($Z \in [-0.42, -0.16]\text{ m}, Y \in [-0.35, 0.05]\text{ m}$), drive heavy bar/weapon forward with both hands ($V_z < -1.15\text{ m/s}$) to wedge the blades (`E` + `Space`) with $80\text{ Hz}$ ($0.95$) metallic impeller jam shockwave.
* **Inmate Pit Brawl Two-Handed Sand Fan Scatter:** While crouched in the pit, fling both open hands outward laterally ($V_{x, left} < -0.80\text{ m/s}, V_{x, right} > +0.80\text{ m/s}$) to scatter a wide cloud of choking dust/ash (`E` + `LMB` + `Space`) with $170\text{ Hz}$ ($0.55$) dual particulate rush haptics.
* **EVA Spacewalk Solar Visor Pull:** Reach above forehead ($Y \in [0.12, 0.30]\text{ m}, Z \in [-0.20, -0.05]\text{ m}$) with Left Grip held and pull downward ($V_y < -0.45\text{ m/s}$) to lower gold-tinted radiation shield (`Q`) with $150\text{ Hz}$ ($0.60$) magnetic latch seal haptics.
* **Medical Syringe Sedative Self-Injection:** Bring injector to off-arm or neck corridor ($X \in [-0.24, -0.06]\text{ m}, Y \in [-0.18, 0.12]\text{ m}$) with dominant grip held and squeeze trigger ($> 0.65$) to inject bio-adrenaline (`E` + `Q`) with $160\text{ Hz}$ pneumatic hiss, $240\text{ Hz}$ needle plunge, and $100\text{ Hz}$ ($0.85$) cardiovascular rush pulse.
* **Atmospheric Hull Rupture Emergency Foam Canister:** In two-handed stance, aim nozzle at hull breach ($Z \in [-0.44, -0.16]\text{ m}$), hold both triggers ($> 0.60$), and sweep across fissure ($|V| > 0.30\text{ m/s}$) to extrude expanding acoustic sealant foam (`E` + `Space`) with $120\text{ Hz}$ ($0.60$) jet hiss and $200\text{ Hz}$ ($0.75$) polymerization cure pop.
* **Thermal Decoy Magnesium Flare Strike:** Grasp flare cylinder with off-hand and striker cap with dominant hand ($dist < 16\text{ cm}$), then shear outward/downward violently ($|\Delta V| > 0.80\text{ m/s}$) to ignite thermal decoy flare (`4` + `LMB`) with $260\text{ Hz}$ ($0.85$) percussion snap and $90\text{ Hz}$ sputtering flame crackle.
* **Inmate Bunk Overhead Pipe Chin-Up:** Reach both hands overhead ($Y > 0.22\text{ m}$) with both grips clenched ($> 0.60$) and pull downward simultaneously ($V_y < -0.50\text{ m/s}$) to hoist body up into ceiling pipe tuck (`Space` + `C`) with dual $75\text{ Hz}$ ($0.70$) iron pipe flex strain haptics.
* **Tactical Laser Sighting Foregrip Pressure Pad:** In two-handed stance, squeeze off-hand trigger ($> 0.50$) or click off-hand thumbstick to toggle laser sight pointer (`F`) with $180\text{ Hz}$ ($0.45$) micro-switch click.
* **Inmate Shiv Parrying Riposte & Eye-Gouge:** From active Boxer Guard, thrust dominant blade upward and forward into eye level ($V_z < -1.10\text{ m/s}, V_y > 1.05\text{ m/s}, Y > 0.04\text{ m}$) for lethal orbital counter-thrust (`W` + `LMB`) with $280\text{ Hz}$ ($0.95$) cartilage perforation haptics.
* **Subterranean Hydroponics Plant Specimen Harvest:** Reach to grow-bed ($Y \in [-0.55, -0.15]\text{ m}, Z \in [-0.42, -0.14]\text{ m}$), pinch grip ($0.45 - 0.80$), and pull upward ($V_y > 0.40\text{ m/s}$) to pluck curative root specimens (`E`) with $160\text{ Hz}$ ($0.45$) stem snap click.
* **Dark Athena Drone Recharge Umbilical Disconnect:** Reach into vertical charging alcove ($Z \in [-0.44, -0.16]\text{ m}, Y \in [0.05, 0.38]\text{ m}$) with grip held and yank backward violently ($V_z > 0.85\text{ m/s}$) to rip high-voltage umbilical connector (`E` + `Space`) with $240\text{ Hz}$ ($0.90$) inductive arc shudder.
* **Heavy Riot Shotgun Breaching Hinge Blast:** In two-handed shotgun stance, place muzzle against door lock ($Z \in [-0.38, -0.16]\text{ m}$), pull trigger ($> 0.80$), and shove forward dynamically ($V_z < -1.20\text{ m/s}$) to blast and kick open fortified doors (`LMB` + `W` + `Space`) with dual $0.98$ concussive hinge-shattering blast ($90\text{ Hz}$).
* **Inmate Pit Brawl Grit Gathering & Concealment:** While crouched near floor ($Y < -0.45\text{ m}$), scoop grit rearward ($V_z > 0.35\text{ m/s}$) with grip held and touch waist pocket to prime blinding dust (`E` + `Tab`) with $180\text{ Hz}$ ($0.45$) cloth pocket rustle confirmation.
* **Cryo-Stasis Chamber Biometric Bypass Override:** Stabilize off-hand at upper bypass slot while pulling lower hydraulic breaker lever downward ($V_y < -0.50\text{ m/s}$) to override locked cryo canopies (`E` + `Enter`) with $120\text{ Hz}$ ($0.80$) magnetic disconnect thud.
* **Dark Athena EVA Micro-Thruster Jetpack Burst:** Extend both arms backward along hips ($Z > -0.06\text{ m}, |X| > 0.16\text{ m}$) and squeeze both triggers ($> 0.65$) to ignite cold-gas micro-jets (`Space` + `W`) with sustained $130\text{ Hz}$ ($0.65$) pneumatic thrust rumble.
* **Inmate Shiv Carotid Arterial Slash:** In stealth crouch behind adversaries, sweep dominant blade horizontally across neck corridor ($|V_x| > 1.25\text{ m/s}, Y \in [-0.12, 0.16]\text{ m}$) for silent lethal jugular incision (`W` + `LMB`) with $260\text{ Hz}$ ($0.95$) arterial release haptics.
* **Dark Athena Fusion Reactor Core Rod Insertion:** In two-handed stance, hoist magnetic containment fuel rod overhead ($Y > 0.15\text{ m}$) and plunge downward into core channel ($V_z < -1.15\text{ m/s}, V_y < -0.80\text{ m/s}$) to lock fuel cell (`E` + `Space` + `Enter`) with $45\text{ Hz}$ ($0.98$) deep sub-bass structural shockwave.
* **Security Guard Taser Wire Snip & Disarm:** When hooked by stun darts, sweep dominant blade across front chest plane ($|V_x| > 0.90\text{ m/s}$) to sever electrified leads and ground circuit (`E` + `3`) with $220\text{ Hz}$ ($0.75$) wire shear snap.
* **Mining Core Heavy Ore Sledgehammer Overhead Crush:** In two-handed stance, hoist sledgehammer high overhead ($Y > 0.28\text{ m}$) and plunge downward violently ($V_y < -1.40\text{ m/s}$) to deliver catastrophic kinetic blunt smash (`S` + `LMB`) with dual $0.98$ concussive fracture shockwaves at $80\text{ Hz}$.
* **Dark Athena Drone Optical Sensor Laser Strike:** In two-handed rifle stance with tactical laser pointer engaged (`Trigger_left > 0.50`), align laser beam on drone sensor optics ($Z \in [-0.45, -0.18]\text{ m}, Y \in [-0.10, 0.30]\text{ m}, |V| < 0.20\text{ m/s}$) for $500\text{ ms}$ dwell to blind optical processor (`F` + `E`) with dual $240\text{ Hz}$ ($0.65$) optic overload pop.
* **Inmate Pit Brawl Ash Two-Handed Fan Clench & Burst:** Bring both clenched fists together at chest ($dist < 18\text{ cm}, Y \in [-0.25, 0.05]\text{ m}$) with both grips clenched ($> 0.60$), then explosively fling arms outward horizontally ($V_{x, left} < -1.10\text{ m/s}, V_{x, right} > +1.10\text{ m/s}$) to disperse a wide-arc blinding dust burst (`E` + `LMB` + `Tab`) with $175\text{ Hz}$ ($0.70$) particulate cloud rush.
* **Subterranean Hydroponics Nutrient Dispenser Plunger:** Grasp vertical pump rod ($Z \in [-0.38, -0.14]\text{ m}, Y \in [-0.15, 0.25]\text{ m}$) with grip held and perform 3 down-strokes ($V_y < -0.45\text{ m/s}$) to clear clogged line and restore nutrient flow (`E` + `Q`) with $110\text{ Hz}$ ($0.60$) hydraulic displacement slosh and $220\text{ Hz}$ ($0.75$) accumulator relief pop.
* **Dark Athena Cargo Crane Overhead Remote Pendant Snatch:** Reach overhead ($Y > 0.18\text{ m}, Z \in [-0.32, -0.05]\text{ m}$) with grip held and haul pendant downward ($V_y < -0.45\text{ m/s}$) to snatch gantry pendant and engage remote bridge (`E` + `W`) with $160\text{ Hz}$ ($0.70$) cable tension & relay click.
* **Heavy Mech Rotary Chaingun Recoil Compensation:** In two-handed braced stance ($dist < 22\text{ cm}$) during sustained automatic fire ($Trigger_{right} > 0.80, Grip_{left} > 0.60, t > 300\text{ ms}$), clamp weapon muzzle climb ($\times 0.35$) with dual harmonic haptics ($160\text{ Hz}$ bolt cycle on dominant hand, $110\text{ Hz}$ hydraulic chassis absorption on off-hand).
* **Mining Rail Turntable Push Lever:** Grasp peripheral lever ($Z \in [-0.45, -0.16]\text{ m}, Y \in [-0.40, -0.05]\text{ m}$) with grip held and haul in 3 tangential horizontal arcs ($|V_x| > 0.50\text{ m/s} \lor |V_z| > 0.50\text{ m/s}$) to rotate turntable platform (`E` + `Space` + `Tab`) with $55\text{ Hz}$ ($0.65$) ball-bearing rumble and $80\text{ Hz}$ ($0.90$) detent clunk.
* **EVA Solar Flare Radiation Shelter Bulkhead Seal:** Raise both hands overhead ($Y > 0.22\text{ m}, Z \in [-0.30, 0.05]\text{ m}$) with both grips clenched ($> 0.60$) and slam downward violently ($V_{y, left} < -1.10\text{ m/s}, V_{y, right} < -1.10\text{ m/s}$) to seal heavy lead radiation door (`E` + `C` + `Space`) with dual $40\text{ Hz}$ ($0.98$) structural impact shockwaves.
* **Inmate Bunk Shiv Sheath Under Mattress Frame:** While physically crouched ($Y_{HMD} < -0.30\text{ m}$), lift mattress edge with off-hand ($Y \in [-0.45, -0.15]\text{ m}$) and thrust dominant hand into hollow iron frame slat ($V_z < -0.65\text{ m/s}$) to conceal handcrafted shiv (`3` + `E`) with $120\text{ Hz}$ ($0.35$) spring wire tension and $240\text{ Hz}$ ($0.60$) spring-steel latch click.
* **Security Guard Riot Shield Wedge Pry & Disarm:** When facing shield-bearing guards, bring both hands forward ($Z \in [-0.42, -0.16]\text{ m}, Y \in [-0.20, 0.20]\text{ m}$), grasp shield rim with off-hand ($Grip > 0.60$), and violently wrench outward in opposing directions ($|\Delta V| > 0.90\text{ m/s}$) to pry the shield away and disarm (`E` + `LMB` + `1`) with $280\text{ Hz}$ ($0.95$) metallic shear crack.
* **Cryo-Chamber Defrost Keypad Poke:** Position fingertip at stasis pod terminal ($Z \in [-0.38, -0.15]\text{ m}, Y \in [-0.15, 0.20]\text{ m}$) with open hand and poke forward 3 times ($V_z < -0.32\text{ m/s}$) to override thermal cycles (`E` + `Enter` + `Q`) with $260\text{ Hz}$ ($0.45$) membrane clicks and $180\text{ Hz}$ ($0.70$) defrost chime.
* **Mining Pneumatic Jackhammer Ground Chisel:** Grasp dual T-bar handles ($dist \in [0.22, 0.45]\text{ m}$) with both grips clenched ($> 0.60$) and plunge downward violently ($V_{y, left} < -1.15\text{ m/s}, V_{y, right} < -1.15\text{ m/s}$) to shatter bedrock escape passages (`S` + `Space` + `LMB`) with dual $50\text{ Hz}$ ($0.98$) seismic percussion shockwaves.
* **Dark Athena EVA Solar Array Hand Crank:** Grasp exterior truss hub ($Z \in [-0.42, -0.16]\text{ m}, Y \in [-0.15, 0.25]\text{ m}$) with both grips held ($> 0.50$) and crank 3 full circular revolutions ($|V| > 0.50\text{ m/s}$) to align stellar collector panels (`E` + `Enter` + `W`) with $150\text{ Hz}$ ($0.50$) gear teeth clicks and $90\text{ Hz}$ ($0.85$) collector lock hum.
* **Inmate Pit Brawl Pocket Ash Throw Riposte:** From active Boxer Guard, dip dominant hand to waist pocket ($Y \in [-0.45, -0.15]\text{ m}, |X| \in [0.10, 0.26]\text{ m}$) with grip held and fling upward ($V_y > 1.10\text{ m/s}, V_z < -0.90\text{ m/s}$) to blind opponent and execute lethal riposte (`E` + `Tab` + `W` + `LMB`) with $220\text{ Hz}$ ($0.85$) strike snap.
* **Subterranean Hydroponics Nutrient Dosing Solenoid Lever:** Grasp wall manifold lever ($Z \in [-0.42, -0.15]\text{ m}, Y \in [-0.25, 0.15]\text{ m}$) with grip held and throw upward ($V_y > 0.65\text{ m/s}$) to prime nutrient feed (`E` + `Space` + `Q`) with $110\text{ Hz}$ ($0.80$) armature clack and fluid pipe surge.
* **Dark Athena EVA Solar Debris Shield Ejection:** Grasp collector truss jettison handles ($Z \in [-0.40, -0.14]\text{ m}, Y \in [0.10, 0.35]\text{ m}$) with both grips clenched ($> 0.55$) and yank backward violently ($V_{z, left} > 0.85\text{ m/s}, V_{z, right} > 0.85\text{ m/s}$) to blow explosive pins (`E` + `Space` + `Enter`) with $240\text{ Hz}$ ($0.95$) pyrotechnic decoupling shockwaves.
* **Tactical Stun Baton Overload Arc Wave Discharge:** Cross batons at chest ($dist < 14\text{ cm}$) while squeezing both grips ($> 0.70$) and triggers ($> 0.65$) for $250\text{ ms}$, then thrust outward violently ($|V_x| > 1.05\text{ m/s} \lor V_z < -1.10\text{ m/s}$) to detonate a 360-degree electrostatic shock ring (`S` + `LMB` + `Space` + `Q`) with dual $300\text{ Hz}$ ($0.98$) concussive arc wave blasts.
* **Tactical Eyeshine & Flashlight:** Tap right temple with grip to toggle Eyeshine (`Q`); tap left shoulder to toggle flashlight (`F`).

---

## 📜 License & Disclaimer

This project is an unofficial community modification. *The Chronicles of Riddick: Escape from Butcher Bay* and *Assault on Dark Athena* are the intellectual property of Starbreeze Studios, Atari, and Universal Studios. No copyrighted game assets or proprietary executables are included in this repository.
