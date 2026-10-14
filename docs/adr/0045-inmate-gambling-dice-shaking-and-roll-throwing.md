# ADR 0045: Inmate Gambling Dice Shaking and Roll Throwing Gestures

## Status
Accepted

## Context
In *The Chronicles of Riddick: Escape from Butcher Bay* (Tower 17 Recreation Area & Mining Camp):
1. **Prison Economy & Gambling:** Inmates gamble UD currency and cigarette packs in clandestine dice games against snitches and cell bosses.
2. **Desktop Mechanics:** Pressing 'E' or mouse click initiated a simple probabilistic menu roll.
3. **VR Physicality:** In VR, cupping the hand, shaking the dice against one's palm with a rhythmic rattle, and rolling them out across the table/floor brings prison underground culture alive.

## Decision

We implement **Kinetic Dice Cup Shaking and Throwing** in [`src/hook/camera_hook.cpp`](file:///C:/Users/GigelCastel/riddick-vr/src/hook/camera_hook.cpp):

```
                   [Player Holding Dice in Cupped Hand]
                   [Left or Right Grip Held > 0.45]
                                  │
                                  ▼
           [Reciprocating Shake Motion: |Speed| > 0.70 m/s]
           - Injects rapid bone-on-bone rattle pulses: 0.30 (160 Hz)
                                  │
                                  ▼
           [Forward Release Throw: Grip Released during Arm Swing]
           - Rolls dice across gambling surface ('E' confirm)
           - Delivers 0.50 release tumble haptic
```

### 1. Reciprocating Shake Rattle
- High-frequency velocity oscillations ($> 0.70\text{ m/s}$) trigger rhythmic micro-taps ($0.30$ at $160\text{ Hz}$), simulating dice rattling in a cupped fist.

### 2. Forward Roll Release
- Releasing grip while swinging forward rolls the dice, confirming the bet and initiating the round.

## Consequences

### Positive
- Deepens inmate interaction and non-combat prison life immersion.
- Highly satisfying tactile feedback for social gambling mini-games.
