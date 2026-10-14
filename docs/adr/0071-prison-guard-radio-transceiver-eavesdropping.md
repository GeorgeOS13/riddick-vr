# ADR 0071: Prison Guard Radio Transceiver Eavesdropping & Intercept Tap

## Status
Accepted

## Context
In *The Chronicles of Riddick: Escape from Butcher Bay* and *Assault on Dark Athena*, security personnel and mercs coordinate search patrols, security lockdowns, and alert states using encrypted tactical radio transceivers. In the flat-screen original, radio chatter was played automatically over 2D audio channels or through subtitle popups.

In VR, stealth immersion is significantly enhanced when the player can actively choose to tune into tactical radio frequencies by bringing their hand or stolen comm-link earpiece directly to their ear, eavesdropping on guard chatter to discern alert statuses, patrol routes, and security passcodes.

## Decision
We implement a physical ear-tap radio transceiver intercept gesture:
1. **Spatial Kinematic Envelope:**
   - Either controller brought into proximity with the corresponding ear/temple ($|X_{local}| > 0.16\text{ m}, Y_{local} \in [0.05, 0.22]\text{ m}, Z_{local} \in [-0.15, +0.08]\text{ m}$).
   - Controller grip clutched ($Grip > 0.50$) simulating holding an earpiece or communicator.
2. **Temporal Hysteresis & Radio Tuning:**
   - Position must be sustained for $\ge 120\text{ ms}$ to engage intercept mode.
   - Re-trigger debounce buffer: $1200\text{ ms}$.
3. **Engine Action Dispatch & Radio Squelch Haptics:**
   - Dispatches PDA log / tactical radio intercept key (`VK_TAB`).
   - Generates radio frequency squelch burst haptics on the listening controller ($0.40$ amplitude at $220\text{ Hz}$ for $150\text{ ms}$) followed by low-amplitude background carrier hum ($0.15$ at $120\text{ Hz}$).

## Consequences
- Elevates stealth gameplay by making patrol intel gathering a deliberate physical action.
- Position bounding ($|X| > 0.16\text{ m}, Y \in [0.05, 0.22]\text{ m}$) cleanly isolates the radio gesture from frontal boxer guard and mouth rebreather stances.
