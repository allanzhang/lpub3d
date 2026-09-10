#!/usr/bin/env python3
"""Seed the translation store from translations that already exist.

lupdate can only carry a translation across when BOTH context and source match
byte for byte. Adding a tr() around a previously hardcoded literal creates a
brand new (context, source) pair, so the identical English text sitting in
another context - already translated - does not help. This script closes that
gap in two passes:

  pass A  import translations lupdate itself produced (same-text heuristic)
          into the store, so they survive the next re-extraction
  pass B  reuse a translation from ANY context with the identical source

Run it right after lupdate + remap_store.py, before translating what is left.

  seed_store.py            # seed
  seed_store.py --dry-run  # report only
"""
from __future__ import annotations

import importlib.util
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
spec = importlib.util.spec_from_file_location("tw", os.path.join(HERE, "ts_work.py"))
tw = importlib.util.module_from_spec(spec)
spec.loader.exec_module(tw)


def collect(store, entries):
    """source -> best known translation, from the store first then the .ts."""
    by_source = {}
    for e in entries:
        t = (store.get(e["id"]) or {}).get("t") or e["existing"]
        if t and e["s"] not in by_source:
            by_source[e["s"]] = t
    return by_source


def main():
    dry = "--dry-run" in sys.argv
    store = tw.load_store()
    root, entries = tw.load_entries(tw.TS_PATH)

    relevant = [e for e in entries
                if not tw.skip_reason(e["s"], e["c"], e["id"])]

    # ---- pass A: harvest what lupdate already filled in -------------------
    harvested = []
    for e in relevant:
        if store.get(e["id"], {}).get("t"):
            continue
        if e["existing"]:
            harvested.append(e)
            if not dry:
                store[e["id"]] = {"t": e["existing"]}

    # ---- pass B: reuse identical source text from another context ---------
    by_source = collect(store, entries)
    reused = []
    for e in relevant:
        if store.get(e["id"], {}).get("t"):
            continue
        t = by_source.get(e["s"])
        if t:
            reused.append((e, t))
            if not dry:
                store[e["id"]] = {"t": t}

    # ---- report ------------------------------------------------------------
    print(f"pass A  从 .ts 已有译文导入 : {len(harvested)}")
    print(f"pass B  跨 context 复用      : {len(reused)}")
    for e, t in reused[:20]:
        print(f"          [{e['c']}] {e['s'][:44]!r} -> {t[:30]!r}")
    if len(reused) > 20:
        print(f"          ... 另 {len(reused) - 20} 条")

    if not dry:
        tw.save_store(store)

    # ---- what is still missing --------------------------------------------
    todo = [e for e in relevant if not store.get(e["id"], {}).get("t")]
    seen = set()
    todo = [e for e in todo if not (e["id"] in seen or seen.add(e["id"]))]
    print(f"\n仍待翻译: {len(todo)} 条，源文合计 {sum(len(e['s']) for e in todo):,} 字符")
    for e in todo:
        print(f"  [{e['c']}] {e['s'][:110]!r}")


if __name__ == "__main__":
    main()
