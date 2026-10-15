# ADR 0404: Sub-Pixel Smooth Turning Accumulation

## Context
1. In `src/hook/camera_hook.cpp`, smooth turn velocity (`curvedSpeed`) was previously cast directly to integer pixel steps (`static_cast<int>(std::copysign(curvedSpeed, stickX))`) on each tick without fractional accumulation.
2. At high refresh rates (90 Hz / 120 Hz) or subtle thumbstick deflections ($|X| \in [0.20, 0.45]$), the calculated displacement per frame was frequently fractional ($< 1.0\text{ px}$). The previous fallback forced a minimum 1-pixel step for any normalized deflection above $0.05$, creating an artificial step-function plateau rather than a true progressive velocity curve, leading to micro-stutter during slow rotational scans.

## Decision
1. **Sub-Pixel Turn Accumulation:**
   - Implemented an accumulator (`static float s_accumulatedSmoothTurnX = 0.0f;`) within the smooth turn handler.
   - Fractional rotation increments are accumulated continuously across frames. Only whole integer mouse deltas are dispatched to `mouse_event(MOUSEEVENTF_MOVE)`, with the remainder carried over into subsequent frames.
   - Reset the accumulator to zero when the stick rests within the deadzone ($|X| \le 0.20$) to prevent residual drift.
2. **Smooth Progressive Ramp:**
   - Applied a quadratic velocity curve ($V = \text{norm}^2 \times 20.0\text{ px/tick}$) with sub-pixel precision for fluid target panning.

## Consequences
- Ultra-smooth rotational tracking across the entire thumbstick deflection range.
- Eliminates quantization micro-jitter during slow, deliberate camera pans.
- Zero drift when thumbstick returns to center deadzone.
