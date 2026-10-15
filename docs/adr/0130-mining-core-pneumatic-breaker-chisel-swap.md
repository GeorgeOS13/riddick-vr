# ADR 0130: Subterranean Mining Core Pneumatic Breaker Chisel Point Swap

## Status
Accepted

## Context
When penetrating reinforced tungsten bulkhead seams or ultra-dense pyroxenite veins in Butcher Bay's lower mine shafts, standard industrial chisel points blunt and shatter. To maintain excavation efficiency and breach security barriers, Riddick must release the pneumatic tool's quick-disconnect chuck collar and seat a hardened diamond point.

## Decision
1. **Two-Handed Driver Chuck Alignment**:
   - Both hands brought together at the driver barrel ($dist(Left, Right) < 0.18\text{ m}$).
   - Off-hand grasps the quick-release spring collar ($Grip_{left} > 0.60$).
2. **Collar Pull & Bit Ramming Stroke**:
   - Off-hand yanks collar rearward ($V_{z, left} > 0.65\text{ m/s}$).
   - Followed by dominant hand plunging forward into battery ($V_{z, right} < -0.80\text{ m/s}$) within $600\text{ ms}$.
3. **Ball-Detent Collar Release & Hardened Steel Lock Haptics**:
   - Release: Spring ball-detent disengagement click ($180\text{ Hz}$, $0.60$, $30\text{ ms}$).
   - Lock: Concussive hardened alloy chuck seating snap ($90\text{ Hz}$, $0.90$, $90\text{ ms}$).
4. **Engine Input Injection**:
   - Dispatches tool bit swap command (`E` + `Space` + `3`) with a $1500\text{ ms}$ lockout.

## Consequences
- Expands two-handed pneumatic tool handling alongside drill percussion (ADR 0037), ground chiseling (ADR 0113), and rock bolting (ADR 0123).
- Delivers a satisfying, mechanically grounded interaction that reinforces Riddick's tactile mastery over industrial machinery.
