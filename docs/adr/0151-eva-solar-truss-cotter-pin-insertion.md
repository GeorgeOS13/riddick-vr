# ADR 0151: Dark Athena EVA Solar Collector Truss Articulation Lock Hinge Cotter Pin Insertion

## Status
Accepted

## Context
Following emergency lubrication or hinge realignment across the vacuum exterior of the *Dark Athena*, articulating solar collector trusses will fail to hold their geometry under thruster acceleration unless mechanically locked with heavy alloy spring cotter pins. In native flat-screen gameplay, securing the structure was automated.

In VR, physical immersion is heightened when the player must align a heavy spring-steel cotter pin with the hinge retention bore and seat it home with an authoritative forward kinetic thrust.

## Decision
We implement **EVA Solar Collector Truss Articulation Lock Hinge Cotter Pin Insertion**:
1. **Hinge Bore Corridor**:
   - Dominant hand positioned at the hinge retention collar ($Z \in [-0.42, -0.15]\text{ m}$, $Y \in [0.10, 0.40]\text{ m}$).
   - Dominant grip held ($> 0.60$) clutching the pin casing.
2. **Dynamic Seating Thrust**:
   - Driving the controller forward into the bore ($V_z < -0.85\text{ m/s}$) compresses the spring detents and seats the pin.
3. **Input Dispatch**:
   - Dispatches `E` (interact) + `Space` (structural clamp) + `Enter` (lock verification).
4. **Haptic Feedback**:
   - Initial $180\text{ Hz}$ ($0.60$) detent compression friction followed by an explosive $280\text{ Hz}$ ($0.95$, $120\text{ ms}$) metallurgical seating snap.

## Consequences
- Completes the full mechanical cycle of EVA hinge maintenance (grease gun injection -> cotter pin lock).
- Debounced with a $1500\text{ ms}$ refractory period.
