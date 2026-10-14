# ADR 0110: Inmate Bunk Shiv Sheath Concealment Under Mattress Frame

## Status
Accepted

## Context
Butcher Bay prison guards execute surprise shakedowns through the cell blocks. Players who carry unauthorized contraband or crafted shivs are subject to immediate confiscation and disciplinary beatings. While ADR 0050 enabled searching bunks to loot hidden items, inmates also need a physical gesture to stow their own weapons into the hollow iron slats of their bunk mattress frame to evade guard detection.

## Decision
1. **Mattress Lift & Bunk Concealment Zone**:
   - Player physically crouched or kneeling near cell bunk ($Y_{HMD} < -0.30\text{ m}$).
   - Off-hand grasping and lifting the mattress edge ($Y_{local, left} \in [-0.45, -0.15]\text{ m}, Grip_{left} > 0.60$).
   - Dominant hand thrusting forward into the hollow frame channel ($Z_{local, right} \in [-0.42, -0.14]\text{ m}, V_{z, right} < -0.65\text{ m/s}$).
2. **Diegetic Contraband Stash Haptics**:
   - Off-hand: $120\text{ Hz}$ ($0.35$) spring wire tension hum during mattress lift.
   - Dominant hand: $240\text{ Hz}$ ($0.60$) crisp spring-steel sheath latch click upon inserting the weapon.
3. **Engine Input Injection**:
   - Dispatches weapon holstering / unequip command (`3` + `E`) to stash weapon safely into the hollow bunk slat with a $1500\text{ ms}$ debounce.

## Consequences
- Deepens prison stealth roleplay and allows players to survive cell inspections without losing handcrafted blades.
- Natural two-handed coordination mirrors the real-world physicality of hiding contraband under prison bedding.
