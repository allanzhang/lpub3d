#!/usr/bin/env python3
"""Verify that a macOS .app bundle is self-contained.

Why this exists
---------------
macdeployqt is not reliably idempotent within a single invocation. Observed on
the myLPub3D v2.5.1 release build (macOS 26 / Qt 6.11.1):

  * plugin dependencies were not deployed - Contents/PlugIns/imageformats/
    libqpdf.dylib still referenced @rpath/QtPdf.framework, which was absent
    from Contents/Frameworks;
  * the foreign LC_RPATH /opt/homebrew/lib survived in the main executable.

Together those two made dyld resolve QtPdf from the build machine's Homebrew,
which dragged in a SECOND copy of QtCore/QtGui/QtNetwork/QtDBus. Duplicate
Objective-C classes then produced the crash the user saw:

  EXC_BAD_ACCESS (SIGSEGV)
  "Thread stack size exceeded due to excessive recursion"
    -[NSWindow _applyWindowLevelWithTagUpdateNeeded:]   x7
    -[NSWindow addChildWindow:ordered:]
    -[NSPopupMenuWindow _commonPresentFromView:animated:]

Re-running macdeployqt fixed both, so the release step must not trust a single
pass: run this check, and if it fails re-run macdeployqt before shipping.

Usage
-----
    python3 verify_bundle.py [--fix] /path/to/myLPub3D.app

Exit status is 0 when the bundle is hermetic, 1 otherwise (details on stdout).
"""
from __future__ import annotations

import os
import re
import subprocess
import sys

SYSTEM_PREFIXES = ("/usr/lib", "/System")
FOREIGN_PREFIXES = ("/opt/homebrew", "/usr/local", "/opt/local")

MAGICS = (b"\xcf\xfa\xed\xfe", b"\xce\xfa\xed\xfe", b"\xca\xfe\xba\xbe")


def macho_files(app: str):
    for dirpath, _dirnames, filenames in os.walk(app):
        for name in filenames:
            path = os.path.join(dirpath, name)
            if os.path.islink(path):
                continue
            try:
                with open(path, "rb") as fh:
                    if fh.read(4) not in MAGICS:
                        continue
            except OSError:
                continue
            yield path


def own_install_name(path: str) -> str:
    """The LC_ID_DYLIB of `path`, or '' when the file carries none.

    Plugins and executables have no LC_ID_DYLIB, so for them the FIRST entry of
    `otool -L` is a genuine dependency, not the file's own install name.
    Skipping that entry unconditionally would hide exactly the failure this
    script exists to catch (libqpdf.dylib -> @rpath/QtPdf.framework).
    """
    out = subprocess.run(["otool", "-D", path], capture_output=True, text=True).stdout
    lines = [l.strip() for l in out.splitlines()[1:] if l.strip()]
    return lines[0] if lines else ""


def dependencies(path: str):
    """Return the dylibs `path` links against, excluding its own install name."""
    out = subprocess.run(["otool", "-L", path], capture_output=True, text=True).stdout
    deps = [l.strip().split(" (")[0] for l in out.splitlines()[1:] if l.strip()]
    own = own_install_name(path)
    if own and deps and deps[0] == own:
        deps = deps[1:]
    return deps


def rpaths(path: str):
    out = subprocess.run(["otool", "-l", path], capture_output=True, text=True).stdout
    return re.findall(r"LC_RPATH\b.*?\n(?:.*\n)*?\s+path (\S+) ", out)


def check(app: str):
    app = os.path.abspath(app)
    frameworks = os.path.join(app, "Contents", "Frameworks")
    macos = os.path.join(app, "Contents", "MacOS")
    bad_deps, bad_rpaths, count = [], [], 0

    for path in macho_files(app):
        count += 1
        rel = os.path.relpath(path, app)
        for dep in dependencies(path):
            if dep.startswith(SYSTEM_PREFIXES):
                continue
            if dep.startswith(("@loader_path", "@executable_path")):
                resolved = (dep.replace("@loader_path", os.path.dirname(path))
                               .replace("@executable_path", macos))
                if not os.path.exists(resolved):
                    bad_deps.append((rel, dep))
            elif dep.startswith("@rpath"):
                name = dep[len("@rpath/"):].split("/")[0]
                if not os.path.exists(os.path.join(frameworks, name)):
                    bad_deps.append((rel, dep))
            elif dep.startswith(FOREIGN_PREFIXES):
                bad_deps.append((rel, dep))
        for rpath in rpaths(path):
            if rpath.startswith(FOREIGN_PREFIXES):
                bad_rpaths.append((rel, rpath))

    return count, bad_deps, bad_rpaths


def strip_foreign_rpaths(app: str, bad_rpaths):
    """Delete foreign LC_RPATH entries. Returns the number removed.

    macdeployqt clears the main executable's foreign rpath but leaves the ones
    baked into third-party dylibs it copied (libjasper/libdbus/libjpeg here), and
    re-running macdeployqt does not help. They are only latent - nothing in a
    correct bundle resolves through them - but on a machine that has Homebrew Qt
    any future unresolved @rpath would silently pull in a second Qt, which is the
    crash this gate exists to prevent. Removing them makes that impossible.

    This mutates the bundle, so it must run BEFORE codesign.
    """
    removed = 0
    for rel, rpath in bad_rpaths:
        path = os.path.join(app, rel)
        res = subprocess.run(["install_name_tool", "-delete_rpath", rpath, path],
                             capture_output=True, text=True)
        if res.returncode == 0:
            removed += 1
        else:
            print(f"   could not remove {rpath} from {rel}: {res.stderr.strip()}")
    return removed


def main(argv):
    args = [a for a in argv[1:] if not a.startswith("-")]
    fix = "--fix" in argv
    if len(args) != 1:
        print("usage: verify_bundle.py [--fix] /path/to/App.app")
        return 2
    app = args[0]
    if not os.path.isdir(os.path.join(app, "Contents")):
        print(f"not an .app bundle: {app}")
        return 1

    count, bad_deps, bad_rpaths = check(app)
    print(f"scanned {count} Mach-O files in {app}")

    if fix and bad_rpaths:
        print(f"removing {len(bad_rpaths)} foreign LC_RPATH entr(y|ies)...")
        removed = strip_foreign_rpaths(app, bad_rpaths)
        print(f"removed {removed}")
        count, bad_deps, bad_rpaths = check(app)

    if not bad_deps and not bad_rpaths:
        print("OK - bundle is self-contained (no unresolved or foreign references)")
        return 0

    if bad_deps:
        print(f"\nUNRESOLVED or FOREIGN dependencies ({len(bad_deps)}):")
        for rel, dep in bad_deps[:20]:
            print(f"   {rel}\n       -> {dep}")
    if bad_rpaths:
        print(f"\nFOREIGN LC_RPATH entries ({len(bad_rpaths)}):")
        for rel, rp in bad_rpaths[:20]:
            print(f"   {rel}\n       -> {rp}")
    if bad_deps:
        print("\nRun macdeployqt again (with -always-overwrite), then re-check.")
    else:
        print("\nRe-run with --fix to strip the foreign rpaths, then re-check.")
    print("A bundle in this state loads a SECOND Qt from the build machine and")
    print("crashes with duplicate Objective-C classes on machines that have Homebrew Qt.")
    return 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
