# myLPub3D

**myLPub3D** is DoubleEagle's customized build of [LPub3D](https://github.com/trevorsandy/lpub3d) — an open source WYSIWYG editor for creating LEGO® style digital building instructions — focused on high-quality rendering of printed/exported instructions.

This fork is maintained as a **single version** project: the `master` branch always tracks the current release, and all DoubleEagle customizations are folded into it (no parallel feature/release branches).

## v2.5.0 (2026-09-07)

- **Sharper renders**: crisp hard edges at any zoom — no line smoothing / export MSAA softening; borders follow the global line-width setting (`3`) for both step images (CSI) and part lists (PLI).
- **Dark-part outlines**: white outlines for dark colors (dark blue / green / brown / olive), pure-black outlines for the rest; defined once, applied to every part.
- **CSI annotations**: ARROW / STEP_BADGE / INSERT ARROW / INSERT PICTURE with full PLACEMENT control (page header/footer relative anchors), baked into the rendered step images.
- **Stability fixes**: Qt 6.11 build fixes, background-thread dialog crash, PDF/export open on macOS/Linux, export page-range parsing ("1 of 2" format), fade/highlight rendering.
- **Product identity**: renamed to myLPub3D with DoubleEagle application identity.

See [CHANGELOG.md](CHANGELOG.md) for details.

## Download

- [myLPub3D-v2.5.0-macOS.zip](https://github.com/allanzhang/lpub3d/releases/download/v2.5.0/myLPub3D-v2.5.0-macOS.zip) (macOS, Apple Silicon)

## Upstream

Original LPub3D is developed and maintained by Trevor SANDY: <https://github.com/trevorsandy/lpub3d>. All improvements made here are layered on top of the upstream project.

## License

myLPub3D is free software released under the [GNU General Public License v3.0 (GPLv3)](https://www.gnu.org/licenses/gpl-3.0.en.html).

## Disclaimer

LEGO® is a trademark of the LEGO Group of companies which does not sponsor, authorize or endorse this application.
