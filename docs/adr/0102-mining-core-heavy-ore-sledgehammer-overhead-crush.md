# ADR 0102: Mining Core Heavy Ore Sledgehammer Two-Handed Overhead Crush

## Status
Accepted

## Context
In Butcher Bay's lower mining depths and Dark Athena's cargo salvage holds, heavy industrial slag sledgehammers provide immense kinetic striking power. While slow and unwieldy, a fully committed two-handed overhead swing generates sufficient destructive force to fracture hardened mineral boulders, demolish security barricades, and shatter armored automated drone carapaces in a single blow.

In flat desktop play, heavy sledgehammer attacks were simply heavy melee attack animations triggered by holding and releasing the mouse button. In VR, hoisting the hammer high above the head with both hands and driving it downward with explosive force delivers unmatched physical power.

## Decision
1. **Two-Handed Heavy Haft Grip**:
   - Both hands gripping the long haft ($dist \in [0.15, 0.40]\text{ m}$) with both grips clenched ($Grip_{left} > 0.55 \land Grip_{right} > 0.55$).
2. **Extreme Overhead Elevation**:
   - Both controllers hoisted high above head level ($Y_{left} > 0.28\text{ m} \land Y_{right} > 0.28\text{ m}$ relative to HMD).
3. **Catastrophic Downward Kinetic Plunge**:
   - Simultaneous downward plunge velocity exceeding heavy thresholds ($V_{y, left} < -1.40\text{ m/s} \land V_{y, right} < -1.40\text{ m/s}$).
4. **Seismic Concussive Ground Shockwave Haptics**:
   - Impact triggers maximum-intensity dual $0.98$ concussive fracture pulses at $80\text{ Hz}$ across both hands ($120\text{ ms}$ duration).
5. **Engine Input Injection**:
   - Injects heavy blunt destruction attack (`S` + `LMB`) with a $1500\text{ ms}$ heavy weapon swing recovery window.

## Consequences
- Creates the ultimate heavy melee tactile experience in the VR port.
- Clear separation from pipe slams (ADR 0026) by requiring higher elevation ($> 0.28\text{ m}$) and higher downward velocity ($< -1.40\text{ m/s}$).
