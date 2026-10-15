"""
Regression test for GH #42 (null-read crash at video init).
Root cause: deferred hook init ran only on first wglCreateContext, so any
earlier Hook_ export (notably wglGetProcAddress, which engines call during
pixel-format/mode setup BEFORE creating a context) ran with null real-function
pointers and returned nullptr -> engine called NULL -> EIP=0 crash.
Invariant: every OpenGLProxy::Hook_ entry point must call
RiddickVR_EnsureHookInitialized() as its first statement (documented
exemptions only). Runs in CI (no game needed).
"""
import re, sys
from pathlib import Path

SRC = Path(__file__).resolve().parent.parent / "src" / "hook" / "opengl_proxy.cpp"
# Entries provably safe without init (pure null-guarded forward, no state).
EXEMPT = {}

def main():
    src = SRC.read_text(encoding="utf-8")
    pat = re.compile(
        r'(?:HGLRC|BOOL|PROC|void|GLboolean|GLuint|GLenum|GLint)\s+WINAPI\s+'
        r'OpenGLProxy::(Hook_\w+)\([^)]*\)\s*\{(.*?)\n\}',
        re.DOTALL,
    )
    missing, found = [], 0
    for m in pat.finditer(src):
        name, body = m.group(1), m.group(2)
        if name in EXEMPT:
            continue
        found += 1
        head = body.split("}")[0][:600]
        if "RiddickVR_EnsureHookInitialized();" not in head:
            missing.append(name)
    print(f"[hook-init] checked {found} Hook_ entries, missing Ensure: {missing or 'none'}")
    if missing:
        print("[hook-init] FAIL: pre-context calls would run uninitialized (GH #42 class)")
        return 1
    print("[hook-init] PASS")
    return 0

if __name__ == "__main__":
    sys.exit(main())
