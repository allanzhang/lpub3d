#!/usr/bin/env python3
"""Classify every LPub3D occurrence in a C/C++ source file.

Walks the file with a small state machine that understands line comments,
block comments and string/char literals, then labels each LPub3D hit as one of:

  comment   - inside // or /* */ (fork markers, prose)
  string    - inside a "..." literal            <- candidate for renaming
  ident     - part of a larger identifier (setLPub3DLoaded, ...)  <- must not change

Usage: classify_lpub3d.py <file> [<file> ...]
       classify_lpub3d.py --summary <roots...>
"""
from __future__ import annotations

import os
import re
import sys
from collections import Counter

NEEDLE = "LPub3D"
ID_CHARS = set("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_")


def classify(path: str):
    try:
        src = open(path, encoding="utf-8", errors="replace").read()
    except OSError:
        return []

    n = len(src)
    i = 0
    state = "code"          # code | line_comment | block_comment | string | char
    hits = []
    while i < n:
        c = src[i]
        nxt = src[i + 1] if i + 1 < n else ""

        if state == "code":
            if c == "/" and nxt == "/":
                state = "line_comment"; i += 2; continue
            if c == "/" and nxt == "*":
                state = "block_comment"; i += 2; continue
            if c == '"':
                state = "string"; start = i + 1; i += 1; continue
            if c == "'":
                state = "char"; i += 1; continue

        elif state == "line_comment":
            if c == "\n":
                state = "code"
            elif src.startswith(NEEDLE, i):
                hits.append(("comment", src[max(0, i - 70):i + 50].splitlines()[-1]))
                i += len(NEEDLE); continue

        elif state == "block_comment":
            if c == "*" and nxt == "/":
                state = "code"; i += 2; continue
            if src.startswith(NEEDLE, i):
                line = src.count("\n", 0, i) + 1
                hits.append(("comment", f"line {line}"))
                i += len(NEEDLE); continue

        elif state == "string":
            if c == "\\":
                i += 2; continue
            if c == '"':
                state = "code"; i += 1; continue
            if src.startswith(NEEDLE, i):
                end = src.find('"', i)
                hits.append(("string", src[start:end if end > 0 else i + 60]))
                i += len(NEEDLE); continue

        elif state == "char":
            if c == "\\":
                i += 2; continue
            if c == "'":
                state = "code"
            i += 1; continue

        # identifier detection applies only in code state
        if state == "code" and src.startswith(NEEDLE, i):
            before = src[i - 1] if i > 0 else ""
            after = src[i + len(NEEDLE)] if i + len(NEEDLE) < n else ""
            kind = "ident" if (before in ID_CHARS or after in ID_CHARS) else "code"
            hits.append((kind, src[max(0, i - 70):i + 50].splitlines()[-1]))
            i += len(NEEDLE); continue

        i += 1
    return hits


def main():
    if "--summary" in sys.argv:
        roots = sys.argv[sys.argv.index("--summary") + 1:]
        tally = Counter()
        samples = {}
        for root in roots:
            for dirpath, _dirs, files in os.walk(root):
                for f in files:
                    if not f.endswith((".cpp", ".h", ".ui", ".pri", ".pro")):
                        continue
                    p = os.path.join(dirpath, f)
                    for kind, ctx in classify(p):
                        tally[kind] += 1
                        samples.setdefault(kind, []).append((p, ctx.strip()))
        print("kind      count")
        print("-" * 22)
        for k, v in tally.most_common():
            print(f"{k:9s} {v}")
        for k in ("string", "code"):
            if samples.get(k):
                print(f"\n--- {k} samples ---")
                for p, ctx in samples[k][:12]:
                    print(f"  {p}: {ctx[:110]}")
        return

    for path in sys.argv[1:]:
        for kind, ctx in classify(path):
            print(f"[{kind:8s}] {ctx.strip()[:130]}")


if __name__ == "__main__":
    main()
