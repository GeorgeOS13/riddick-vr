"""
Autonomous Self-Play Testing Pipeline for Riddick VR
Runs the game and simulated VR companion host headlessly,
captures stereo GPU eye frames, inspects HUD positioning and aspect ratio,
and validates gameplay stability without requiring a physical VR headset.
"""

import os
import sys
import time
import subprocess
import argparse
from pathlib import Path
from PIL import Image
import numpy as np

GAME_DIR = Path(r"F:\The Chronicles of Riddick - Assault on Dark Athena\System\Win32_x86")
GAME_EXE = GAME_DIR / "DarkAthena.exe"
HOST_EXE = Path(r"C:\Users\GigelCastel\riddick-vr\dist\riddickvr-host.exe")
LOG_FILE = GAME_DIR / "riddick_vr.log"
DEFAULT_ARTIFACTS_DIR = Path(r"C:\Users\GigelCastel\riddick-vr\tests\artifacts")


def clean_artifacts(artifacts_dir: Path):
    artifacts_dir.mkdir(parents=True, exist_ok=True)
    for f in artifacts_dir.glob("*.bmp"):
        try:
            f.unlink()
        except Exception:
            pass
    for f in artifacts_dir.glob("*.png"):
        try:
            f.unlink()
        except Exception:
            pass


def analyze_frame(bmp_path: Path):
    """Inspects a captured VR eye buffer for aspect ratio, HUD visibility, and visual defects."""
    img = Image.open(bmp_path)
    width, height = img.size
    arr = np.array(img.convert("RGB"))

    # 1. Total resolution check (Meta Quest 2 per-eye buffer)
    res_ok = (width == 2496 and height == 2688)

    # 2. Check for non-black active rendered area
    # Compute brightness across horizontal and vertical slices
    row_max = arr.max(axis=(1, 2))
    active_rows = np.where(row_max > 10)[0]

    col_max = arr.max(axis=(0, 2))
    active_cols = np.where(col_max > 10)[0]

    is_black = (len(active_rows) == 0 or len(active_cols) == 0)

    active_top = int(active_rows[0]) if len(active_rows) > 0 else 0
    active_bottom = int(active_rows[-1]) if len(active_rows) > 0 else height - 1
    active_left = int(active_cols[0]) if len(active_cols) > 0 else 0
    active_right = int(active_cols[-1]) if len(active_cols) > 0 else width - 1

    active_h = active_bottom - active_top + 1
    active_w = active_right - active_left + 1
    active_aspect = active_w / max(active_h, 1)
    is_loading_indicator = is_black or (active_w < 100 or active_h < 100)

    # 3. Check aspect ratio (should be ~1.78 for 16:9, or loading frame)
    aspect_ok = True if is_loading_indicator else (1.45 <= active_aspect <= 1.95)

    # 4. Check HUD position:
    # In Quest 2 2688-tall buffer, the safe viewing zone is y in [600, 2100].
    # Bottom HUD elements should be situated above y = 2200 (not clipped off at y = 2688).
    hud_safe_bottom = True if is_loading_indicator else (active_bottom < 2200)

    # 5. Check for Chroma-key Green Flashes (#00FF00)
    green_mask = (arr[:, :, 1] > 200) & (arr[:, :, 0] < 50) & (arr[:, :, 2] < 50)
    green_pixel_count = int(np.sum(green_mask))
    no_green_glitches = (green_pixel_count < 50)

    # Convert to PNG for compact storage and inspection
    png_path = bmp_path.with_suffix(".png")
    img.save(png_path, "PNG")

    passed = aspect_ok and hud_safe_bottom and no_green_glitches

    return {
        "filename": bmp_path.name,
        "png_path": str(png_path),
        "width": width,
        "height": height,
        "is_black": is_black,
        "is_loading_indicator": is_loading_indicator,
        "active_bounds": (active_left, active_top, active_right, active_bottom),
        "active_w": active_w,
        "active_h": active_h,
        "aspect_ratio": round(active_aspect, 3) if not is_loading_indicator else 0.0,
        "aspect_ok": aspect_ok,
        "hud_safe_bottom": hud_safe_bottom,
        "no_green_glitches": no_green_glitches,
        "green_pixel_count": green_pixel_count,
        "passed": passed,
    }


def run_pipeline(max_frames=120, dump_interval=15, timeout=25, artifacts_dir=DEFAULT_ARTIFACTS_DIR):
    print("=" * 60)
    print("  RIDDICK VR: AUTONOMOUS SELF-PLAY TESTING PIPELINE")
    print("=" * 60)

    clean_artifacts(artifacts_dir)
    print(f"[Pipeline] Output artifacts directory: {artifacts_dir}")

    # 1. Start mock OpenXR companion host
    host_cmd = [
        str(HOST_EXE),
        "--mock",
        "--auto-exit-with-game",
        "--dump-dir", str(artifacts_dir),
        "--dump-interval", str(dump_interval),
        "--max-frames", str(max_frames),
    ]
    print(f"[Pipeline] Launching mock VR host: {' '.join(host_cmd)}")
    host_proc = subprocess.Popen(
        host_cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        bufsize=1,
    )

    time.sleep(1.5)

    # 2. Launch game DarkAthena.exe
    print(f"[Pipeline] Launching game: {GAME_EXE}")
    game_proc = subprocess.Popen(
        [str(GAME_EXE)],
        cwd=str(GAME_DIR),
    )

    # 3. Monitor execution
    start_time = time.time()
    host_logs = []
    print("[Pipeline] Running autonomous test session...")

    while True:
        elapsed = time.time() - start_time
        if host_proc.poll() is not None:
            print(f"[Pipeline] Host finished after {elapsed:.1f}s (Exit code: {host_proc.returncode})")
            break

        if game_proc.poll() is not None:
            print(f"[Pipeline] Game exited early after {elapsed:.1f}s (Exit code: {game_proc.returncode})")
            break

        if elapsed > timeout:
            print(f"[Pipeline] Test timeout ({timeout}s) reached. Stopping processes...")
            break

        # Non-blocking read of host stdout
        line = host_proc.stdout.readline()
        if line:
            line_str = line.strip()
            host_logs.append(line_str)
            if "Heartbeat" in line_str or "Captured test frame" in line_str or "Bound GPU" in line_str:
                print(f"  [Host Log] {line_str}")

        time.sleep(0.05)

    # Cleanup processes
    try:
        if game_proc.poll() is None:
            game_proc.terminate()
            game_proc.wait(timeout=3)
    except Exception:
        subprocess.run(["taskkill", "/F", "/IM", "DarkAthena.exe"], capture_output=True)

    try:
        if host_proc.poll() is None:
            host_proc.terminate()
            host_proc.wait(timeout=3)
    except Exception:
        subprocess.run(["taskkill", "/F", "/IM", "riddickvr-host.exe"], capture_output=True)

    # 4. Analyze generated frame screenshots
    bmp_files = sorted(artifacts_dir.glob("*.bmp"))
    print("\n" + "=" * 60)
    print(f"  TEST ARTIFACTS ANALYSIS ({len(bmp_files)} frames captured)")
    print("=" * 60)

    if not bmp_files:
        print("[Pipeline] ERROR: No test frames captured! Checking logs...")
        if LOG_FILE.exists():
            print(f"[Pipeline] riddick_vr.log tail:\n" + LOG_FILE.read_text()[-1000:])
        return False

    all_passed = True
    rendered_count = 0
    for bmp_file in bmp_files:
        res = analyze_frame(bmp_file)
        if res["is_black"] or res["is_loading_indicator"]:
            print(f"\nFrame: {res['filename']} (Initial Startup / Black / Loading Spinner Transition Frame -> PASS)")
            continue

        rendered_count += 1
        print(f"\nFrame: {res['filename']} -> PNG: {res['png_path']}")
        print(f"  - Resolution:   {res['width']}x{res['height']} (Expected 2496x2688)")
        print(f"  - Active Box:   {res['active_bounds']} (Width: {res['active_w']}, Height: {res['active_h']})")
        print(f"  - Aspect Ratio: {res['aspect_ratio']} (Expected 16:9, ~1.78) -> {'PASS' if res['aspect_ok'] else 'FAIL'}")
        print(f"  - HUD Safe Y:   Bottom at {res['active_bounds'][3]} (< 2200 lens sweet spot) -> {'PASS' if res['hud_safe_bottom'] else 'FAIL'}")
        print(f"  - Green Glitch: {res['green_pixel_count']} green pixels -> {'PASS' if res['no_green_glitches'] else 'FAIL'}")

        if not res["passed"]:
            all_passed = False

    if rendered_count == 0:
        print("\n[Pipeline] WARN: All captured frames were black loading frames.")
        all_passed = False

    print("\n" + "=" * 60)
    print(f"  OVERALL RESULT: {'PASSED (READY FOR PLAY)' if all_passed else 'NEEDS ATTENTION'}")
    print("=" * 60)
    return all_passed


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Autonomous Self-Play VR Test Runner")
    parser.add_argument("--frames", type=int, default=100, help="Number of test frames")
    parser.add_argument("--interval", type=int, default=15, help="Frame capture interval")
    parser.add_argument("--timeout", type=int, default=25, help="Test timeout in seconds")
    args = parser.parse_args()

    success = run_pipeline(
        max_frames=args.frames,
        dump_interval=args.interval,
        timeout=args.timeout,
    )
    sys.exit(0 if success else 1)
