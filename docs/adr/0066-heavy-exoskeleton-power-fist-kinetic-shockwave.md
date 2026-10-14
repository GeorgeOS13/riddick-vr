# ADR 0066: Heavy Exoskeleton Power Fist Kinetic Shockwave Punch

## Status
Accepted / Implemented

## Context
Piloting the Heavy Exoskeleton Combat Mech in *Assault on Dark Athena* transforms Riddick into a walking armored tank. While rotary chainguns shred distant units, close-quarters combat against heavy blast doors, security barricades, and enemy mechs relies on hydraulic pneumatic power fists capable of generating concussive kinetic shockwaves.

In VR, winding up a physical straight punch and unleashing an explosive forward thrust should transmit the immense power of pressurized industrial hydraulics through the player's controllers.

## Decision
We implement exoskeleton power fist shockwave punches in Section 5a of `src/hook/camera_hook.cpp`:
1. **Mechanical Windup & High Kinetic Velocity**:
   - Player pulls dominant hand back to shoulder ($Z_{local} > 0.05\text{ m}$).
   - Throws an explosive punch exceeding high velocity threshold:
     $V_{z, right} < -1.60\text{ m/s}$.
   - Controller grip engaged ($> 0.60$).
2. **Hydraulic Kinetic Discharge**:
   - Dispatches Primary Attack and forward concussive impulse (`LMB` + `'W'`), obliterating lightweight barricades and staggering heavy mechs.
3. **Concussive Hydraulic Haptics**:
   - Emits a dual $0.98$ amplitude low-frequency ($100\text{ Hz}$) shockwave pulse simulating hydraulic piston recoil across both arms.

## Consequences
- Conveys the colossal mass and mechanical force of piloting combat mechs in VR.
- High velocity threshold ($1.60\text{ m/s}$) differentiates power fist strikes from standard human punches.
- Deep, low-frequency haptics ($100\text{ Hz}$) contrast sharply with human bone-cracking strikes ($280\text{ Hz}$).
