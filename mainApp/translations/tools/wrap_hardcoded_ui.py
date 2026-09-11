#!/usr/bin/env python3
"""Find and fix hardcoded user-visible UI literals that lupdate cannot see.

lupdate only extracts strings passed through tr() / translate(). Anything handed
straight to a user-facing setter - a menu title, a dialog title, a button label -
is invisible to it and stays English forever, no matter how complete the .ts
looks. This script finds that whole class of gap and wraps it.

    wrap_hardcoded_ui.py mainApp lclib           # list what would change
    wrap_hardcoded_ui.py --apply mainApp lclib   # rewrite the sources

After a rewrite, always:
    lupdate ... && seed_store.py     # new entries need translations
    make                             # the compiler is the safety net (see below)

Three traps this script is built around:

1. Line endings. Several sources in this tree are CRLF. Reading with text-mode
   open() normalises CRLF to LF and the rewrite becomes a whole-file diff that
   buries the real change (28k lines, observed). Everything here goes through a
   byte-level round trip instead.

2. Classes that are not QObject. QGraphicsItem and friends have no tr(); using it
   does not compile. Those call sites need an explicit context instead:
       QCoreApplication::translate("ClassName", "literal")
   This script cannot tell the two apart, so it wraps everything and lets the
   compiler point at the ones to convert. Match the context to the class name -
   that is what lupdate assigns for a class-based tr(), so existing translations
   stay valid and no re-translation is needed.

3. Multi-fragment literals. A literal split over several lines
       setWhatsThis("first part\n" "second part")
   would be wrapped as tr("first part\n") "second part" - a syntax error. The
   compiler catches it; the fix is to move the closing paren past the last
   fragment. The scan at the end of this file looks for leftovers.
"""
from __future__ import annotations

import os
import re
import sys

SETTERS = ["setText", "setTitle", "addMenu", "setWindowTitle", "setToolTip",
           "setStatusTip", "setWhatsThis", "setLabelText", "setPlaceholderText",
           "setTabText", "setInformativeText", "setDetailedText",
           "setWindowIconText", "setAccessibleName", "setAccessibleDescription",
           "setItemText", "setHeaderLabel", "setHorizontalHeaderLabels",
           "setVerticalHeaderLabels", "setButtonText", "setPrefix", "setSuffix",
           "setSpecialValueText", "setSubTitle", "setCancelButtonText"]
CTORS = ["QMenu", "QAction", "QPushButton", "QCheckBox", "QRadioButton",
         "QLabel", "QGroupBox", "QToolButton", "QTabWidget", "QDialog"]

OPENER = (r"(?P<open>(?:\b(?:" + "|".join(SETTERS) + r")\s*\()"
          r"|(?:\bnew\s+(?:" + "|".join(CTORS) + r")\s*\()"
          r"|(?:\baddAction\s*\())")
CALL_RE = re.compile(OPENER + r'(?P<lit>"(?:[^"\\]|\\.)*")')

# literals that are deliberately not translatable prose
EXCLUDE_LITERALS = {"1234", "<b>Z:</b>"}
EXCLUDE_RE = re.compile(r"^<span\s+style=")          # HTML style wrappers
# a bare markup fragment ("<h3>", "</b>", "<br>") concatenated around real
# prose - wrapping it would add noise entries, not translatable text
EXCLUDE_TAG_RE = re.compile(r"^</?[a-zA-Z][^>]*>$")
# a literal that is only an identifier / path / format token
ALLOW_RE = re.compile(
    r"^$"
    r"|^[a-z][A-Za-z0-9_]*$"                          # camelCase ids (QUndoCommand::setText)
    r"|^[\s\-_.:/|*+=<>]*$"
    r"|^https?://"
    r"|^[A-Za-z0-9_.\-/]+\.(png|jpg|jpeg|bmp|ico|icns|ui|qrc|cpp|h|dat|ldr|mpd"
    r"|ini|xml|lst|pov|obj|3ds|dae|stl|csv|txt|zip|qm|ts|sh|bat|py|json|yml)$"
)

# leftover `tr(...)` immediately followed by a bare literal = broken concatenation
BROKEN_RE = re.compile(r'tr\s*\((?:[^()"]|"(?:[^"\\]|\\.)*")*\)\s*"', re.S)


def wrap_file(path, apply):
    # Byte-level round trip - see trap 1 in the module docstring.
    try:
        src = open(path, "rb").read().decode("utf-8")
    except UnicodeDecodeError:
        # Non-UTF-8 third-party sources (e.g. LDView's tinyxml xmltest.cpp,
        # which is ISO-8859-1 on purpose) carry no user-visible UI strings.
        return []
    out, changed = [], 0
    for m in CALL_RE.finditer(src):
        lit = m.group("lit")
        body = lit[1:-1]
        if re.search(r"tr\s*\(\s*$", src[max(0, m.start("open") - 40):m.start("open")]):
            continue
        if body in EXCLUDE_LITERALS or EXCLUDE_RE.match(body) \
           or EXCLUDE_TAG_RE.match(body) or ALLOW_RE.match(body):
            continue
        ln = src.count("\n", 0, m.start()) + 1
        out.append((ln, body))
        changed += 1

    if apply and out:
        # rebuild from the end so offsets stay valid
        for m in reversed(list(CALL_RE.finditer(src))):
            lit = m.group("lit")
            body = lit[1:-1]
            if re.search(r"tr\s*\(\s*$", src[max(0, m.start("open") - 40):m.start("open")]):
                continue
            if body in EXCLUDE_LITERALS or EXCLUDE_RE.match(body) \
               or EXCLUDE_TAG_RE.match(body) or ALLOW_RE.match(body):
                continue
            src = (src[:m.end("open")] + "tr(" + lit + ")"
                   + src[m.end("lit"):])
        open(path, "wb").write(src.encode("utf-8"))
    return out


def scan_broken(roots):
    """Report `tr(...)` swallowed only part of a concatenated literal."""
    hits = 0
    for root in roots:
        for dp, _d, fs in os.walk(root):
            for f in fs:
                if not f.endswith((".cpp", ".h")):
                    continue
                p = os.path.join(dp, f)
                try:
                    src = open(p, "rb").read().decode("utf-8")
                except UnicodeDecodeError:
                    continue  # non-UTF-8 third-party source, see wrap_file()
                for m in BROKEN_RE.finditer(src):
                    ln = src.count("\n", 0, m.start()) + 1
                    print(f"  拼接断链 {p}:{ln}: {src[m.start():m.end() + 24]!r}")
                    hits += 1
    return hits


def main():
    apply = "--apply" in sys.argv
    roots = [a for a in sys.argv[1:] if not a.startswith("--")]
    total = 0
    for root in roots:
        files = []
        for dp, _d, fs in os.walk(root):
            for f in fs:
                if f.endswith((".cpp", ".h")):
                    files.append(os.path.join(dp, f))
        for f in sorted(files):
            hits = wrap_file(f, apply)
            if hits:
                print(f"  {f}  ({len(hits)})")
                if not apply:
                    for ln, body in hits[:200]:
                        print(f"      {ln}: {body!r}")
                total += len(hits)
    print(f"\n{'已包裹' if apply else '待包裹'} {total} 处")

    broken = scan_broken(roots)
    print(f"拼接断链残留: {broken} 处")
    if apply:
        print("\n下一步：重新编译。编译器会指出哪些调用点位于非 QObject 类，"
              "把它们改成 QCoreApplication::translate(\"类名\", ...)，"
              "然后 lupdate + seed_store.py。")


if __name__ == "__main__":
    main()
