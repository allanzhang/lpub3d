# myLPub3D

**myLPub3D** is DoubleEagle's customized build of [LPub3D](https://github.com/trevorsandy/lpub3d) — an open source WYSIWYG editor for creating LEGO® style digital building instructions — focused on high-quality rendering of printed/exported instructions.

This fork is maintained as a **single version** project: the `master` branch always tracks the current release, and all DoubleEagle customizations are folded into it (no parallel feature/release branches).

## v2.6.1 (2026-09-11)

- **Vector-redrawn splash**: background and text are now separate — the resource is a pure background image and all branding (title, subtitle, rule, version, copyright) is drawn at runtime as vector text, crisp at any DPI; version and year are read from build macros; the subtitle is translatable.
- **Dialog layout guard**: an application-level `DialogSizeGuard` event filter grows any dialog whose designed initial size is smaller than its content minimum (no more clipped labels / overlapping rows); layout-less dialogs with a clean vertical child stack are adopted into a box layout at show time.
- **One button style in every dialog**: ~100 in-dialog QToolButtons converted to QPushButton (67 .ui nodes + ~33 code sites, colour swatches included), ending the mixed bezel/borderless looks under the macOS native style.
- **Localization fixes**: BOM/PLI sort keys translated with display/token separation (file format untouched) plus an upstream initial-selection index bug fix; "Open With Programs Setup" Browse buttons translated and column widths fixed; `Program %1` default names translatable with migration of stored English defaults.

## v2.6.0 (2026-09-11)

- **Simplified Chinese UI**: full localization of the application interface and Qt's standard dialogs — menus, toolbars, status bar, dialogs, context menus and all tooltip / WhatsThis help text (7,379 of 7,379 translatable entries). The language follows the system locale and can be forced with `LPUB3D_LANGUAGE=zh_CN`.
- **Chinese splash screen**: the startup splash subtitle now reads 「LDRAW 拼搭说明书」.
- **Crash fix — no more second Qt**: the packaged app used to load a duplicate copy of Qt from the build machine's Homebrew (macdeployqt had left plugin frameworks undeployed and kept a foreign `-rpath /opt/homebrew/lib`), and the duplicate Objective-C classes crashed it when a menu was opened. The bundle is now self-contained, and every release is gated on `builds/macx/verify_bundle.py`.
- **Removed**: the waiting-spinner dependency and its call sites.
- **App icon**: new myLPub3D macOS application icon, shipped as a full 16–1024 px icon set.

## v2.5.1 (2026-09-09)

- **App icon**: new myLPub3D macOS application icon, shipped as a full 16–1024 px icon set.

## v2.5.0 (2026-09-07)

- **Sharper renders**: crisp hard edges at any zoom — no line smoothing / export MSAA softening; borders follow the global line-width setting (`3`) for both step images (CSI) and part lists (PLI).
- **Dark-part outlines**: white outlines for dark colors (dark blue / green / brown / olive), pure-black outlines for the rest; defined once, applied to every part.
- **CSI annotations**: ARROW / STEP_BADGE / INSERT ARROW / INSERT PICTURE with full PLACEMENT control (page header/footer relative anchors), baked into the rendered step images.
- **Stability fixes**: Qt 6.11 build fixes, background-thread dialog crash, PDF/export open on macOS/Linux, export page-range parsing ("1 of 2" format), fade/highlight rendering.
- **Product identity**: renamed to myLPub3D with DoubleEagle application identity.

See [CHANGELOG.md](CHANGELOG.md) for details.

## Download

- [myLPub3D-v2.6.1-macOS.zip](https://github.com/allanzhang/lpub3d/releases/download/v2.6.1/myLPub3D-v2.6.1-macOS.zip) (macOS, Apple Silicon)
- [myLPub3D-v2.6.0-macOS.zip](https://github.com/allanzhang/lpub3d/releases/download/v2.6.0/myLPub3D-v2.6.0-macOS.zip) (macOS, Apple Silicon)
- [myLPub3D-v2.5.0-macOS.zip](https://github.com/allanzhang/lpub3d/releases/download/v2.5.0/myLPub3D-v2.5.0-macOS.zip) (macOS, Apple Silicon)

## Upstream

Original LPub3D is developed and maintained by Trevor SANDY: <https://github.com/trevorsandy/lpub3d>. All improvements made here are layered on top of the upstream project.

## License

myLPub3D is free software released under the [GNU General Public License v3.0 (GPLv3)](https://www.gnu.org/licenses/gpl-3.0.en.html).

## Disclaimer

LEGO® is a trademark of the LEGO Group of companies which does not sponsor, authorize or endorse this application.
