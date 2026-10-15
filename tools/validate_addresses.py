"""
Track C #34: sigscan table scaffold validator (self-test only).
Loads config/addresses.json, validates schema, and logs the
base+offset resolution math against a placeholder module base so the
REA workflow is exercisable without the game running:

    resolved = module_base + offset

Does NOT attach to any process and does NOT replace keystrokes.
Usage: python3 tools/validate_addresses.py [--table config/addresses.json]
"""
import argparse
import json
import sys
from pathlib import Path

REQUIRED_TOP = ("game", "game_version_pin", "entries")
REQUIRED_ENTRY = ("id", "module", "pattern", "offset", "kind", "status")
PLACEHOLDER_BASE = 0x400000  # typical x86 DarkAthena.exe image base (self-test only)


def validate(table_path: Path) -> bool:
    print(f"[rea] table: {table_path}")
    try:
        table = json.loads(table_path.read_text(encoding="utf-8"))
    except Exception as e:
        print(f"[rea] FAIL: cannot parse JSON: {e}")
        return False

    ok = True
    for key in REQUIRED_TOP:
        if key not in table:
            print(f"[rea] FAIL: missing top-level key '{key}'")
            ok = False
    entries = table.get("entries", [])
    if not entries:
        print("[rea] FAIL: no entries")
        ok = False

    seen = set()
    for i, e in enumerate(entries):
        tag = e.get("id", f"entry[{i}]")
        for key in REQUIRED_ENTRY:
            if key not in e:
                print(f"[rea] FAIL: {tag} missing '{key}'")
                ok = False
        if tag in seen:
            print(f"[rea] FAIL: duplicate id '{tag}'")
            ok = False
        seen.add(tag)
        try:
            offset = int(e.get("offset", 0))
        except (TypeError, ValueError):
            print(f"[rea] FAIL: {tag} has non-integer offset")
            ok = False
            continue
        resolved = PLACEHOLDER_BASE + offset
        print(f"[rea] self-test {tag:24s} module={e.get('module'):16s} "
              f"base=0x{PLACEHOLDER_BASE:08X} offset=0x{offset:08X} "
              f"resolved=0x{resolved:08X} status={e.get('status')}")

    print(f"[rea] game pin: {table.get('game_version_pin')}")
    print(f"[rea] result: {'PASS' if ok else 'FAIL'} "
          f"({len(entries)} entries; keystrokes remain the active path)")
    return ok


if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("--table", default="config/addresses.json")
    args = ap.parse_args()
    sys.exit(0 if validate(Path(args.table)) else 1)
