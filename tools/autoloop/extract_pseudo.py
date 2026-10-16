"""Extract Ghidra pseudocode from a `rea function --format json` capture.

Usage: python tools/autoloop/extract_pseudo.py <in.json> [out.txt]

`rea function` nests the decompiler listing somewhere under its Evidence
envelope; this walks the tree, picks the longest code-looking string and writes
it verbatim (plus a short header naming the subject).  Read-only helper for the
autoloop; writes only the caller-supplied output path.
"""
import json
import sys


def _code_score(s: str) -> int:
    if not isinstance(s, str) or len(s) < 40:
        return 0
    score = s.count(';') + 3 * s.count('{') + 3 * s.count('}')
    if 'FUN_' in s or 'undefined' in s or 'uint' in s or 'adecompiler' in s.lower():
        score += 10
    return score


def _walk(node, out):
    if isinstance(node, dict):
        for v in node.values():
            _walk(v, out)
    elif isinstance(node, list):
        for v in node:
            _walk(v, out)
    elif isinstance(node, str):
        out.append(node)


def main() -> int:
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    src = sys.argv[1]
    dst = sys.argv[2] if len(sys.argv) > 2 else src.rsplit('.', 1)[0] + '.pseudo.txt'
    data = None
    for enc in ('utf-8-sig', 'utf-16'):
        try:
            with open(src, 'r', encoding=enc) as fh:
                data = json.load(fh)
            break
        except (UnicodeDecodeError, json.JSONDecodeError):
            continue
    if data is None:
        print(f'could not decode {src} as utf-8 or utf-16 JSON')
        return 1

    strings = []
    _walk(data, strings)
    strings = [s for s in strings if _code_score(s)]
    if not strings:
        print('no pseudocode-like string found')
        return 1
    best = max(strings, key=_code_score)

    subject = ''
    for key in ('subject', 'target', 'parameters', 'operation'):
        def find(o, k):
            if isinstance(o, dict):
                if k in o and not isinstance(o[k], (dict, list)):
                    return o[k]
                for v in o.values():
                    r = find(v, k)
                    if r is not None:
                        return r
            elif isinstance(o, list):
                for v in o:
                    r = find(v, k)
                    if r is not None:
                        return r
            return None
        got = find(data, key)
        if isinstance(got, dict):
            got = got.get('name') or got.get('local_path')
        if got:
            subject = f'{key}={got}'
            break

    with open(dst, 'w', encoding='utf-8') as fh:
        fh.write(f'# Ghidra pseudocode via rea function --format json ({subject})\n')
        fh.write(best)
    print(f'wrote {dst} ({len(best)} bytes)')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
