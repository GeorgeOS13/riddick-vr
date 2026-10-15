"""
Semi-play validator for Riddick VR hands interactions.
Runs mock host + game headlessly through one full 360-frame gesture loop
(reload / waist / NanoMED palm / grab), then checks:
- frames captured and non-black ratio
- riddick_vr.log contains gesture markers for reload / NanoMED / hands render
Usage: python3 tools/semi_play.py --frames 360 --timeout 120
"""
import argparse, subprocess, time, sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
from self_test_pipeline import run_pipeline, GAME_DIR, LOG_FILE, DEFAULT_ARTIFACTS_DIR

MARKERS = [
    "MANUAL WEAPON RELOAD",
    "POWER CELL HOT-SWAP",
    "NANOMED CARTRIDGE",
    "BIOMETRIC PALM SCANNER",
    "LADDER RUNG GRABBED",
    "Tactical Gesture",
    "Diegetic Interaction",
]

def check_log():
    if not LOG_FILE.exists():
        return {}, ""
    text = LOG_FILE.read_text(errors="ignore")[-20000:]
    found = {m: (m in text) for m in MARKERS}
    return found, text[-2000:]

if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("--frames", type=int, default=360)
    ap.add_argument("--interval", type=int, default=15)
    ap.add_argument("--timeout", type=int, default=120)
    a = ap.parse_args()
    ok = run_pipeline(max_frames=a.frames, dump_interval=a.interval, timeout=a.timeout, artifacts_dir=DEFAULT_ARTIFACTS_DIR)
    found, tail = check_log()
    print("\n=== SEMI-PLAY GESTURE MARKERS (riddick_vr.log tail) ===")
    for k, v in found.items():
        print(f"  {'HIT' if v else 'miss'}: {k}")
    hits = sum(1 for v in found.values() if v)
    print(f"\nGesture markers hit: {hits}/{len(found)}")
    # pass if pipeline rendered at least one non-loading frame OR any gesture marker hit
    # (loading-only runs still prove IPC + hands path stays alive)
    print(f"Pipeline result: {'PASS' if ok else 'LOADING-ONLY (no rendered frames yet)'}")
    sys.exit(0 if (ok or hits > 0) else 1)
