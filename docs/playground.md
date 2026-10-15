# Riddick VR Playground: arena test guide (GH #48)

Goal: assess every ported VR mechanic in ONE quiet trip instead of playing
the campaign. Use the cell-block / yard area after the intro as the arena.

## Station checklist (in order)

1. **Hands check** - open palms, make fists, point index. Expect: slim
   prisoner hands, sleeve cuffs. Firing grip fades overlay to 25%.
2. **Reloads** - off-hand to weapon receiver (<20cm, upward seat) then waist
   pouch. Expect: `MANUAL WEAPON RELOAD` + 0.55/0.75 haptics. Try power-cell
   flick down for the hot-swap eject.
3. **Shiv / melee** - equip slot 3, cross-slash + punch. Expect: no camera
   drift while swinging (GH #50), guard on left trigger with NO spin (#51).
4. **Mob / guard** - throat-grip a guard (forward reach + left grip), shake to
   intimidate; drag the body while crouched.
5. **Heal** - draw cartridge from waist, slam into overhead NanoMED slot,
   then flat palm on scanner. Expect: cartridge visual in palm, heal glow.
6. **Climb** - reach overhead + grip a rung. Hang still: within 1.5s you get
   the PULL-DOWN hint pulse. Pull one hand: ready double-pulse. Pull the
   other within the window: mantle.
7. **Comfort** - strafe + snap/smooth turn. Expect: peripheral vignette while
   moving, off at rest. Tune via `riddick_vr.ini`.
8. **Menus vs gameplay** - open any menu: expect letterboxed cinema screen.
   Back in 3D: expect edge-to-edge, no bars.

## Reporting
Paste `riddick_vr.log` lines with `BarScan:`, `DisplayMode:`, `Climb Hint:`,
`Standing eye height`, plus headset + what you saw per station above.
