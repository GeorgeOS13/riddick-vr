"""GH #40: ammo-quad content verifier (game-free, display-stack-down safe).

Scores whether the fixed ammo-crop rect used by the host actually contains
HUD-like content in captured eye frames:

    x0 = 0.75*W, y0 = 0.75*H, w = 0.23*W, h = 0.23*H
    (src/host/openxr_manager.cpp:1291-1294, weaponAmmoQuadLayer imageRect)

For each frame in tests/artifacts/*.png the tool extracts that crop and
scores HUD-likeness from three stdlib+numpy signals:

  1. bright-pixel ratio  - fraction of crop pixels with luma > 128
     (HUD digits/backplates are bright-on-dark; Riddick scenes are dark).
  2. edge density        - fraction of crop pixels with numpy-gradient
     magnitude > 25 (glyph strokes give dense edges; flat scene fill does not).
  3. digit-blob count    - connected bright regions (threshold > 150) whose
     pixel area falls in a plausible glyph range (pure-python flood fill,
     no scipy; crop is downscaled so this stays fast).

Per-frame verdicts:
  DARK/UNKNOWN - crop mean luma < 12 (black / loading frame, nothing to judge).
  LIKELY-HUD   - bright_ratio >= 0.03 AND edge_density >= 0.03 AND blobs >= 1.
  BACKGROUND   - rendered content in the crop but no HUD structure.

Thresholds are heuristics tuned for bright-on-dark ammo glyphs, NOT ground
truth. A LIKELY-HUD verdict means "the crop rect caught structured bright
content", not "the digits read correctly".

Usage:
  python3 tools/analyze_ammo.py [--dir tests/artifacts]
  python3 tools/analyze_ammo.py --dir C:\\path\\to\\frames

Exit 0 when scoring completes (including zero frames found - the display
stack may be down, which is expected, not a failure). Exit 1 only on a
hard error (unreadable directory, corrupt image).
"""

# GH #40 FOLLOW-UP PROPOSAL (comment only - no behavior change):
# How semi_play.py could auto-capture a receiver-gaze frame later, once the
# display stack is back:
#   1. Drive the mock host to a "receiver-gaze" pose: position the right-hand
#      aim pose so the weapon backplate offset (+0.025m Y, +0.060m Z per
#      openxr_manager.cpp:1256) sits ~0.35m in front of the HMD with its quad
#      normal facing the eyes (ammoGlanceDot > 0.35, same ~70deg cone gate).
#   2. Hold that pose for N frames (>= dump-interval) so the host emits at
#      least one dumped eye frame with the ammo quad layer active, e.g. via
#      run_pipeline(max_frames=..., dump_interval=1) with a --gaze-hold flag.
#   3. Save the dump PNGs into tests/artifacts/ as today, then invoke this
#      analyzer (import analyze_ammo.score_crop / main) as a post-pass and
#      fail the run only if zero frames score LIKELY-HUD while the gaze-hold
#      was active. Keep the game-free SKIP semantics when no frames exist.

import argparse
import sys
from collections import deque
from pathlib import Path

import numpy as np
from PIL import Image

# Must mirror src/host/openxr_manager.cpp:1291-1294 exactly.
CROP_X0 = 0.75
CROP_Y0 = 0.75
CROP_W = 0.23
CROP_H = 0.23

BRIGHT_LUMA = 128      # bright-pixel threshold (0-255 luma)
BLOB_LUMA = 150        # connected-region threshold (stricter: glyph cores)
EDGE_MAG = 25.0        # gradient-magnitude threshold for edge density
DARK_MEAN = 12.0       # below this crop mean luma -> DARK/UNKNOWN
HUD_BRIGHT_MIN = 0.03  # min bright-pixel ratio for LIKELY-HUD
HUD_EDGE_MIN = 0.03    # min edge density for LIKELY-HUD
BLOB_DOWNSCALE = 256   # max crop side (px) used for blob counting
BLOB_MIN_FRAC = 0.0002  # min blob area as fraction of (downscaled) crop
BLOB_MAX_FRAC = 0.25    # max blob area as fraction of (downscaled) crop

DEFAULT_ARTIFACTS_DIR = Path(__file__).resolve().parent.parent / "tests" / "artifacts"


def crop_rect(w, h):
    """Fixed ammo-crop rect in pixels. Returns (x0, y0, x1, y1), clamped."""
    x0 = int(w * CROP_X0)
    y0 = int(h * CROP_Y0)
    x1 = min(w, x0 + int(w * CROP_W))
    y1 = min(h, y0 + int(h * CROP_H))
    return (x0, y0, x1, y1)


def count_blobs(mask, min_area, max_area):
    """Count 4-connected True regions with area in [min_area, max_area]."""
    h, w = mask.shape
    seen = np.zeros_like(mask, dtype=bool)
    blobs = 0
    for y in range(h):
        for x in range(w):
            if not mask[y, x] or seen[y, x]:
                continue
            # Flood fill one region.
            area = 0
            queue = deque([(y, x)])
            seen[y, x] = True
            while queue:
                cy, cx = queue.popleft()
                area += 1
                for ny, nx in ((cy - 1, cx), (cy + 1, cx), (cy, cx - 1), (cy, cx + 1)):
                    if 0 <= ny < h and 0 <= nx < w and mask[ny, nx] and not seen[ny, nx]:
                        seen[ny, nx] = True
                        queue.append((ny, nx))
            if min_area <= area <= max_area:
                blobs += 1
    return blobs


def score_crop(crop):
    """Score one RGB crop array. Returns dict of metrics + verdict."""
    arr = np.asarray(crop, dtype=np.uint8)
    # Rec.601 luma; keep float for gradient.
    gray = (0.299 * arr[:, :, 0] + 0.587 * arr[:, :, 1] + 0.114 * arr[:, :, 2])
    mean_luma = float(gray.mean())
    bright_ratio = float(np.mean(gray > BRIGHT_LUMA))

    gy, gx = np.gradient(gray.astype(np.float32))
    mag = np.hypot(gx, gy)
    edge_density = float(np.mean(mag > EDGE_MAG))

    # Downscale for blob counting (keeps pure-python flood fill cheap).
    h, w = gray.shape
    scale = min(1.0, BLOB_DOWNSCALE / max(h, w))
    if scale < 1.0:
        small = np.asarray(
            Image.fromarray(gray.astype(np.uint8)).resize(
                (max(1, int(w * scale)), max(1, int(h * scale))), Image.BILINEAR
            )
        ).astype(np.float32)
    else:
        small = gray
    sh, sw = small.shape
    total = sh * sw
    blob_mask = small > BLOB_LUMA
    blobs = count_blobs(blob_mask, BLOB_MIN_FRAC * total, BLOB_MAX_FRAC * total)

    if mean_luma < DARK_MEAN:
        verdict = "DARK/UNKNOWN"
    elif bright_ratio >= HUD_BRIGHT_MIN and edge_density >= HUD_EDGE_MIN and blobs >= 1:
        verdict = "LIKELY-HUD"
    else:
        verdict = "BACKGROUND"
    return {
        "mean_luma": round(mean_luma, 2),
        "bright_ratio": round(bright_ratio, 4),
        "edge_density": round(edge_density, 4),
        "blobs": blobs,
        "verdict": verdict,
    }


def analyze_frame(png_path):
    """Load one eye frame, extract the ammo crop, score it."""
    img = Image.open(png_path).convert("RGB")
    w, h = img.size
    x0, y0, x1, y1 = crop_rect(w, h)
    crop = img.crop((x0, y0, x1, y1))
    res = score_crop(np.array(crop))
    res.update({"filename": png_path.name, "size": f"{w}x{h}", "crop": (x0, y0, x1, y1)})
    return res


def main(artifacts_dir):
    print("=" * 72)
    print("  RIDDICK VR: AMMO-QUAD CROP ANALYZER (GH #40 groundwork)")
    print(f"  crop: x0=0.75W y0=0.75H w=0.23W h=0.23H (openxr_manager.cpp:1291-1294)")
    print(f"  dir:  {artifacts_dir}")
    print("=" * 72)
    if not artifacts_dir.is_dir():
        print(f"[ammo] no artifacts directory: {artifacts_dir} (display stack down?)")
        print("[ammo] result: NO-FRAMES (nothing to score; not a failure)")
        return 0
    frames = sorted(artifacts_dir.glob("*.png"))
    if not frames:
        print("[ammo] no *.png frames found (display stack down?)")
        print("[ammo] result: NO-FRAMES (nothing to score; not a failure)")
        return 0

    print(f"[ammo] scoring {len(frames)} frame(s):")
    print(f"  {'frame':36s} {'crop rect':22s} {'mean':>6s} {'bright':>7s} "
          f"{'edge':>6s} {'blobs':>5s}  verdict")
    print("  " + "-" * 100)
    tallies = {"LIKELY-HUD": 0, "BACKGROUND": 0, "DARK/UNKNOWN": 0}
    failed = False
    for f in frames:
        try:
            r = analyze_frame(f)
        except Exception as e:
            print(f"  {f.name:36s} ERROR: {e}")
            failed = True
            continue
        tallies[r["verdict"]] = tallies.get(r["verdict"], 0) + 1
        x0, y0, x1, y1 = r["crop"]
        print(f"  {r['filename']:36s} ({x0},{y0})-({x1},{y1}) {x1-x0}x{y1-y0}"
              f" {r['mean_luma']:6.1f} {r['bright_ratio']:7.4f}"
              f" {r['edge_density']:6.4f} {r['blobs']:5d}  {r['verdict']}")

    total = len(frames)
    print("  " + "-" * 100)
    print(f"[ammo] summary: {total} frame(s): " + ", ".join(
        f"{k}={v}" for k, v in tallies.items()))
    if tallies.get("LIKELY-HUD", 0) > 0:
        print("[ammo] result: HUD-EVIDENCE (crop rect caught structured bright content)")
    elif tallies.get("BACKGROUND", 0) > 0:
        print("[ammo] result: NO-HUD-STRUCTURE (rendered content, no glyph structure in crop)")
    else:
        print("[ammo] result: ALL-DARK (frames are black/loading; crop unjudgeable)")
    return 1 if failed else 0


if __name__ == "__main__":
    ap = argparse.ArgumentParser(description="Score ammo-quad crop HUD-likeness (GH #40).")
    ap.add_argument("--dir", "--artifacts", dest="dir", default=str(DEFAULT_ARTIFACTS_DIR),
                    help="directory of captured eye-frame PNGs")
    args = ap.parse_args()
    sys.exit(main(Path(args.dir)))
