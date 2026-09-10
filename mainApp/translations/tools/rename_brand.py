#!/usr/bin/env python3
"""Rename the user-visible product name LPub3D -> myLPub3D, safely.

Only occurrences INSIDE string literals are considered, and only when the
literal is genuine product-name text. Everything else is deliberately skipped:

  * /*** LPub3D Mod ***/ fork markers and all comments  (structural, track the fork)
  * C++ identifiers such as setLPub3DLoaded()           (would break compilation)
  * lookup keys: ThemeDefaultDecorateLPub3DLocal, editLPub3DIniFileAct.1
  * LDraw meta-command tokens: LPub3D_Fade_, LPub3D_Highlight_, !COLOUR docs
  * the syntax-highlight regex \\bLPub3D_[A-Za-z|_]+\\b
  * filename filter patterns: log (LPub3DLog*);;stderr (stderr-*)
  * upstream attribution / URLs (trevorsandy.github.io)
  * anything already written as myLPub3D

Usage:
  rename_lpub3d.py --check   mainApp lclib     # dry run, list decisions
  rename_lpub3d.py --apply   mainApp lclib     # write changes
"""
from __future__ import annotations

import os
import re
import sys

NEEDLE = "LPub3D"
NEW = "myLPub3D"
EXTS = (".cpp", ".h", ".ui")

# A literal is protected when it looks like an identifier, a lookup key, a
# file-format token, a filter pattern or attribution rather than prose.
PROTECT_PATTERNS = [
    re.compile(r"LPub3D_"),                       # LDraw macro / colour name prefix
    re.compile(r"LPub3DLog"),                     # log file name pattern
    re.compile(r";;\s*(stderr|stdout)"),          # file dialog filter strings
    re.compile(r"trevorsandy"),                   # upstream URL / attribution
    re.compile(r"editLPub3DIniFileAct"),          # Qt objectName lookup key
    re.compile(r"Theme(Default|Dark)Decorate"),   # theme colour table keys
    re.compile(r"LPub3DDataPath"),                # identifier
    re.compile(r"\\b|\[\^|\(\?"),                 # looks like a regex
]

# Standalone LPub3D not already preceded by "my".
RE_STANDALONE = re.compile(r"(?<!my)LPub3D")


def scan(path):
    """Yield (line_no, literal_start, literal_end, literal) for LPub3D literals."""
    src = open(path, encoding="utf-8", errors="replace").read()
    n = len(src)
    i = 0
    state = "code"
    out = []
    while i < n:
        c = src[i]
        nxt = src[i + 1] if i + 1 < n else ""
        if state == "code":
            if c == "/" and nxt == "/":
                state = "line"; i += 2; continue
            if c == "/" and nxt == "*":
                state = "block"; i += 2; continue
            if c == '"':
                start = i + 1; state = "string"; i += 1; continue
            if c == "'":
                state = "char"; i += 1; continue
        elif state == "line":
            if c == "\n":
                state = "code"
        elif state == "block":
            if c == "*" and nxt == "/":
                state = "code"; i += 2; continue
        elif state == "string":
            if c == "\\":
                i += 2; continue
            if c == '"':
                lit = src[start:i]
                if NEEDLE in lit:
                    out.append((src.count("\n", 0, start) + 1, start, i, lit))
                state = "code"; i += 1; continue
        elif state == "char":
            if c == "\\":
                i += 2; continue
            if c == "'":
                state = "code"
            i += 1; continue
        i += 1
    return src, out


def decide(lit: str):
    """Return (should_rename, reason)."""
    if not RE_STANDALONE.search(lit):
        return False, "already myLPub3D"
    for pat in PROTECT_PATTERNS:
        if pat.search(lit):
            return False, f"protected ({pat.pattern[:28]})"
    return True, "display text"


RE_UI_STRING = re.compile(r"<string>(.*?)</string>", re.S)


def process_ui(path, apply_changes, changed, skipped):
    """Handle .ui files, where display text lives in <string> elements rather
    than in C++ string literals - the C++ scanner would never see it."""
    src = open(path, encoding="utf-8", errors="replace").read()
    edits = []
    for m in RE_UI_STRING.finditer(src):
        lit = m.group(1)
        if NEEDLE not in lit:
            continue
        ok, reason = decide(lit)
        if not ok:
            skipped += 1
            if not apply_changes:
                print(f"  SKIP  {path}  [{reason}]  {lit[:80]!r}")
            continue
        new_lit = RE_STANDALONE.sub(NEW, lit)
        edits.append((m.start(1), m.end(1), lit, new_lit))
    if not edits:
        return changed, skipped
    for s0, s1, lit, new_lit in edits:
        print(f"  {'CHANGE' if apply_changes else 'WOULD'} {path}")
        print(f"         - {lit[:110]!r}")
        print(f"         + {new_lit[:110]!r}")
        changed += 1
    if apply_changes:
        out = src
        for s0, s1, lit, new_lit in sorted(edits, reverse=True):
            out = out[:s0] + new_lit + out[s1:]
        open(path, "w", encoding="utf-8").write(out)
    return changed, skipped


def process(root, apply_changes):
    changed = skipped = 0
    for dirpath, _d, files in os.walk(root):
        for f in sorted(files):
            if not f.endswith(EXTS):
                continue
            path = os.path.join(dirpath, f)
            if path.endswith(".ui"):
                changed, skipped = process_ui(path, apply_changes, changed, skipped)
                continue
            src, hits = scan(path)
            if not hits:
                continue
            edits = []
            for line, s0, s1, lit in hits:
                ok, reason = decide(lit)
                if ok:
                    new_lit = RE_STANDALONE.sub(NEW, lit)
                    edits.append((s0, s1, lit, new_lit, line))
                else:
                    skipped += 1
                    if not apply_changes:
                        print(f"  SKIP  {path}:{line}  [{reason}]  {lit[:80]!r}")
            if not edits:
                continue
            for s0, s1, lit, new_lit, line in edits:
                print(f"  {'CHANGE' if apply_changes else 'WOULD'} {path}:{line}")
                print(f"         - {lit[:110]!r}")
                print(f"         + {new_lit[:110]!r}")
                changed += 1
            if apply_changes:
                # rebuild from the end so earlier offsets stay valid
                out = src
                for s0, s1, lit, new_lit, _line in sorted(edits, reverse=True):
                    out = out[:s0] + new_lit + out[s1:]
                open(path, "w", encoding="utf-8").write(out)
    print(f"\n{'changed' if apply_changes else 'would change'}: {changed}   protected: {skipped}")


if __name__ == "__main__":
    mode = "--apply" if "--apply" in sys.argv else "--check"
    roots = [a for a in sys.argv[1:] if not a.startswith("--")]
    process_one = process
    for r in roots:
        print(f"\n===== {r} =====")
        process_one(r, mode == "--apply")
