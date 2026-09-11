#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""审计修复：迁移已失效的译文键 + 修正拼接病句与标点一致性。

1. rekey  — 源码修正后 lupdate 不再产生 QObject::QObject / QMessageBox::QMessageBox
             两个 context，把这 6 条译文从旧键搬到新键，避免丢失。
2. fixes  — 散文拼接产生的病句，以及半角/全角标点混用。
"""
import importlib.util, sys

spec = importlib.util.spec_from_file_location("tw", "mainApp/translations/tools/ts_work.py")
tw = importlib.util.module_from_spec(spec); spec.loader.exec_module(tw)

# ---------- 1. 迁移键 ----------
REKEY = [
    ("QObject::QObject", "QObject", [
        "Add Next Step", "Add Next Steps...", "Add Previous Step",
        "Remove this Step", "Add %1",
    ]),
    ("QMessageBox::QMessageBox", "QMessageBox", [
        "Unsupported LDraw file extension %1 specified.  File not saved.",
    ]),
]

# ---------- 2. 内容修复 ----------
# 按源文替换（同一源文在各 context 统一）
FIX = {
    # --- 拼接病句：ldrawfilesload.cpp 的 "parts not found" 消息 ---
    # 模板 "未找到 %1 %2。" + 片段 was/were→"被" 得到 "未找到 某些零件 被。"
    # 改为把否定交给片段：模板 "%1 %2找到。" + 片段 "未" → "某些零件 未找到。"
    "<br><br>%1 %2 not found. The following locations were searched;<br>"
    "model file, LDraw search paths, %3 and %4 library archives.<br>"
    "If %5 custom %6, be sure %7 location is in the LDraw search directory list.<br>"
    "If %5 new unofficial %6, be sure the unofficial archive library is up to date.":
        "<br><br>%1 %2找到。已搜索以下位置；<br>"
        "模型文件、LDraw 搜索路径、%3 和 %4 库归档。<br>"
        "如果 %5 自定义 %6，请确保 %7 位置在 LDraw 搜索目录列表中。<br>"
        "如果 %5 新的非官方 %6，请确保非官方归档库是最新的。",

    # --- 标点一致性：中文后统一用全角冒号 ---
    ", Build: %1 %2":                          "，构建：%1 %2",
    ", Detected: %1":                          "，检测到：%1",
    "Blender Addon Install Arguments: %1 %2":  "Blender 插件安装参数：%1 %2",
    "%1 Run: Exception %2 has been thrown.":   "%1 运行：抛出了异常 %2。",
    "%1 Run: An unhandled exception has been thrown.":
                                               "%1 运行：抛出了未处理的异常。",
    "%1 Script: %2":                           "%1 脚本：%2",
    "%1 Command: %2":                          "%1 命令：%2",
    "Error: Addon install failed":             "错误：插件安装失败",
    "Error: Install failed.":                  "错误：安装失败。",
    "Find: ":                                  "查找：",
    "Replace: ":                               "替换：",
    "Durat's Stl library: %1":                 "Durat 的 Stl 库：%1",
    "POVRay include path: %1":                 "POVRay 包含路径：%1",

    # --- 术语一致：与 lcModelListDialog 对齐（该操作是"复制"）---
    "Duplicate Submodel":                      "复制 Sub-model",
}

# 必须限定 context 的修正：同一个英文片段在不同类里语义不同。
# Gui 的 was/were 用在 "There %1 %2%3 :" 里，译作"有 "才对；
# QObject 的用在 "%1 %2 not found" 里，需译作"未"。
FIX_SCOPED = {
    ("QObject", "was"):  "未",
    ("QObject", "were"): "未",
}


def main():
    dry = "--dry-run" in sys.argv
    store = tw.load_store()
    print(f"store 修复前: {len(store)} 条")

    moved = 0
    for old_ctx, new_ctx, sources in REKEY:
        for s in sources:
            oid, nid = tw.uid(old_ctx, s), tw.uid(new_ctx, s)
            rec = store.get(oid)
            if not rec or not rec.get("t"):
                print(f"  !! 旧键无译文: [{old_ctx}] {s[:40]!r}")
                continue
            if nid in store and store[nid].get("t") != rec["t"]:
                print(f"  !! 目标键已存在且不同: [{new_ctx}] {s[:40]!r}")
                continue
            if not dry:
                store[nid] = {"t": rec["t"]}
                del store[oid]
            moved += 1
            print(f"  ✓ [{old_ctx}] → [{new_ctx}]  {s[:44]!r}")

    # 内容修复：遍历 .ts 全部条目，按源文匹配
    root, entries = tw.load_entries(tw.TS_PATH)
    fixed = []
    for e in entries:
        newt = FIX.get(e["s"])
        if newt is None:
            newt = FIX_SCOPED.get((e["c"], e["s"]))
        if newt is None:
            continue
        cur = (store.get(e["id"]) or {}).get("t") or e["existing"]
        if not cur or cur == newt:
            continue
        if not dry:
            store[e["id"]] = {"t": newt}
        fixed.append((e["c"], e["s"], cur, newt))

    print(f"\n迁移 {moved} 条；内容修复 {len(fixed)} 条")
    for c, s, old, new in fixed:
        print(f"  [{c}]\n    源文 {s[:64]!r}\n    旧   {old[:64]!r}\n    新   {new[:64]!r}")

    if not dry:
        tw.save_store(store)
        print(f"\nstore 修复后: {len(store)} 条")
    else:
        print("\n[dry-run] 未写盘")


if __name__ == "__main__":
    main()
