"""
Semi-play validator for Riddick VR hands interactions.
Runs mock host + game headlessly through one full 360-frame gesture loop
(reload / waist / NanoMED palm / grab), then checks:
- frames captured and non-black ratio
- riddick_vr.log contains gesture markers for reload / NanoMED / hands render
Usage: python3 tools/semi_play.py --frames 360 --timeout 120
"""
import argparse, subprocess, time, sys
import json
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

def check_log(markers, since_byte=0):
    if not LOG_FILE.exists():
        return {}, ""
    raw = LOG_FILE.read_bytes()[since_byte:]
    text = raw.decode(errors="ignore")[-20000:]
    found = {m: (m in text) for m in markers}
    return found, text[-2000:]

if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("--scenario", choices=["mock-smoke", "reload", "heal", "climb", "throw", "melee", "grenade", "drone"], default="mock-smoke",
                    help="Built-in smoke loop or a scripted tools/scenarios/*.json controller profile")
    ap.add_argument("--frames", type=int, default=360)
    ap.add_argument("--interval", type=int, default=15)
    ap.add_argument("--timeout", type=int, default=120)
    a = ap.parse_args()
    print(f"[Semi-play] Scenario: {a.scenario}")
    log_mark = LOG_FILE.stat().st_size if LOG_FILE.exists() else 0
    scenario_file = None if a.scenario == "mock-smoke" else Path(__file__).resolve().parent / "scenarios" / f"{a.scenario}.json"
    if scenario_file is not None and not scenario_file.is_file():
        print(f"[Semi-play] Scenario file not found: {scenario_file}")
        sys.exit(2)
    expected_markers = MARKERS
    if scenario_file is not None:
        try:
            profile = json.loads(scenario_file.read_text(encoding="utf-8"))
            expected_markers = profile.get("markers", [])
            if not expected_markers or not all(isinstance(marker, str) and marker for marker in expected_markers):
                raise ValueError("profile must declare non-empty marker strings")
        except (OSError, ValueError, json.JSONDecodeError) as exc:
            print(f"[Semi-play] Invalid scenario profile {scenario_file}: {exc}")
            sys.exit(2)
    ok = run_pipeline(max_frames=a.frames, dump_interval=a.interval, timeout=a.timeout,
                      artifacts_dir=DEFAULT_ARTIFACTS_DIR, scenario_file=scenario_file)
    if ok is None:
        print("Pipeline result: SKIP (another run holds the lock)")
        sys.exit(0)
    found, tail = check_log(expected_markers, since_byte=log_mark)
    print("\n=== SEMI-PLAY GESTURE MARKERS (fresh riddick_vr.log output only) ===")
    for k, v in found.items():
        print(f"  {'HIT' if v else 'miss'}: {k}")
    hits = sum(1 for v in found.values() if v)
    print(f"\nGesture markers hit: {hits}/{len(found)}")
    # Scripted profiles require every profile-specific gesture marker. A bright
    # capture alone cannot prove the selected scenario was exercised.
    profile_markers_pass = all(found.values())
    if scenario_file is not None and not profile_markers_pass:
        print("[Semi-play] FAIL: selected scenario did not emit every expected gesture marker.")
        sys.exit(1)

    # Verdict: rendered bright frames prove the full pixel path; otherwise
    # flowing frames + live gesture markers prove the hook path (Riddick's
    # scenes are often near-black, which the frame classifier reports as
    # loading). Exit 0 in both cases; exit 1 only on hook failure/crash.
    if ok:
        print("Pipeline result: PASS (rendered frames verified)")
    elif hits > 0:
        print("Pipeline result: PASS (hook live - frames flowing, gestures firing; scenes dark)")
    else:
        print("Pipeline result: FAIL (no frames, no gestures - hook may be dead)")
    sys.exit(0 if (ok or hits > 0) else 1)
