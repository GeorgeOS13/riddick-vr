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
* **Tactical Eyeshine & Flashlight:** Tap right temple with grip to toggle Eyeshine (`Q`); tap left shoulder to toggle flashlight (`F`).

---

## 📜 License & Disclaimer

This project is an unofficial community modification. *The Chronicles of Riddick: Escape from Butcher Bay* and *Assault on Dark Athena* are the intellectual property of Starbreeze Studios, Atari, and Universal Studios. No copyrighted game assets or proprietary executables are included in this repository.
