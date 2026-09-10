#!/usr/bin/env python3
"""Re-key the translation store after a source-string rename.

The store is keyed by sha1(context + source). Renaming LPub3D -> myLPub3D inside
a tr()'d string changes its source, so its key changes and the translation would
be orphaned. This script carries every translation across by matching old and new
entries on the renamed form of the source.

Order of operations:
  1. lupdate regenerates lpub3d_zh_CN.ts from the renamed sources
  2. this script maps old entries -> new entries and rewrites the store
"""
from __future__ import annotations

import importlib.util
import json
import os
import re
import sys
import xml.etree.ElementTree as ET

HERE = os.path.dirname(os.path.abspath(__file__))
TOOLS = "/Users/allan/Documents/Work/IO Enhancement/myLPub3D/mainApp/translations/tools"
spec = importlib.util.spec_from_file_location("tw", os.path.join(TOOLS, "ts_work.py"))
tw = importlib.util.module_from_spec(spec)
spec.loader.exec_module(tw)

RE_STANDALONE = re.compile(r"(?<!my)LPub3D")


def rename_text(s: str) -> str:
    return RE_STANDALONE.sub("myLPub3D", s)


def parse_ts(path):
    """Return [(context, source, translation)] for every message."""
    root = ET.parse(path).getroot()
    out = []
    for ctx in root.findall("context"):
        name = ctx.findtext("name") or ""
        for msg in ctx.findall("message"):
            src = msg.findtext("source") or ""
            tr = msg.find("translation")
            out.append((name, src, (tr.text or "") if tr is not None else ""))
    return out


def main():
    old_ts, new_ts = sys.argv[1], sys.argv[2]
    store = tw.load_store()
    print(f"store entries before : {len(store)}")

    old_entries = parse_ts(old_ts)
    new_entries = parse_ts(new_ts)

    # index the old entries by their renamed form
    old_index = {}
    for ctx, src, _tr in old_entries:
        old_index[(ctx, rename_text(src))] = tw.uid(ctx, src)

    new_store = {}
    carried = renamed = orphaned = 0
    orphans = []
    for ctx, src, _tr in new_entries:
        new_id = tw.uid(ctx, src)
        old_id = old_index.get((ctx, src))
        if old_id is None:
            # source unchanged, same key
            if new_id in store:
                rec = store[new_id]
                new_store[new_id] = {"t": rename_text(rec["t"])}
                carried += 1
            continue
        rec = store.get(old_id)
        if not rec or not rec.get("t"):
            continue
        new_store[new_id] = {"t": rename_text(rec["t"])}
        if old_id != new_id:
            renamed += 1
        carried += 1

    # anything left in the old store that produced no match
    for k, v in store.items():
        if k not in old_index.values() and k not in new_store:
            orphaned += 1
            orphans.append((k, v.get("t", "")[:60]))

    tw.save_store(new_store)
    print(f"carried over         : {carried}")
    print(f"  of which re-keyed  : {renamed}")
    print(f"store entries after  : {len(new_store)}")
    print(f"orphaned             : {orphaned}")
    for k, t in orphans[:10]:
        print(f"   {k}  {t!r}")


if __name__ == "__main__":
    main()
