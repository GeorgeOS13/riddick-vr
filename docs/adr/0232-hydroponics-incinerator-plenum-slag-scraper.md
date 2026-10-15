# ADR 0232: Subterranean Hydroponics Bio-Waste Incinerator High-Pressure Primary Air Plenum Slag Agitator Scraping Bar

## Status
Accepted

## Context
Vitified biomass slag deposits accumulate over the primary combustion grate and air blast tuyeres within the subterranean incinerator plenum. Inserting a heavy refractory scraping bar through the cleanout port dislodges clinker and restores combustion airflow across the bio-waste conversion chamber.

## Decision
Implement physical plenum slag scraping bar mechanics in `src/hook/camera_hook.cpp`:
- **Two-Handed Scraper Tool Grip**: Grasp the scraper bar shaft with both hands (`handsDistSledge` between $0.16\text{ m}$ and $0.38\text{ m}$, left grip $> 0.55$, right grip $> 0.55$) in front of the cleanout port (`rightLocal.z` between $-0.45\text{ m}$ and $-0.15\text{ m}$, `rightLocal.y` between $-0.30\text{ m}$ and $0.20\text{ m}$).
- **Reciprocating Scraping Strokes**: Drive the bar back and forth with vigorous stroke velocity (`|rVel.z| > 0.65\text{ m/s} \land |lVel.z| > 0.55\text{ m/s}`).
- **Multi-Stroke Grate Clearing**: Accumulate 3 full reciprocating strokes to dislodge packed slag.
- **Synthesized Controls**: Emits `E` + `Q` + `Tab` + `Space` to plunge through crust, scrape tuyere vents, and actuate the grate drop lever.
- **Haptic Feedback**: $300\text{ Hz}$ ($0.95$) refractory clinker impact on the right hand and $160\text{ Hz}$ ($0.85$) resonant shaft vibration on the left hand.

## Consequences
- Completes the operational maintenance cycle of the subterranean bio-waste incinerator primary plenum.
- Works in tandem with dog-clamp dogging, ash winch hoisting, and draft fan venting.
