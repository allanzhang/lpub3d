## Why

myLPub3D cannot open Studio 2.0 `.io` project packages, so models that use
project-local custom parts appear incomplete even though Studio renders them
without installing those parts globally. The current `.ldr` export contains
only custom-part references, not the part definitions packaged inside the
`.io`, so the complete build is not visible in myLPub3D.

## What Changes

- Add an `Open IO File...` entry and accept `.io` and `.mo` files in the file
  chooser and drag-and-drop flow.
- Import a Studio project package without modifying the source archive.
- Read the package with the Studio default archive password when a password is
  required, and do not expose that password in logs or user-facing errors.
- Prefer the standard `model.ldr` as the base model for myLPub3D.
- Build a temporary self-contained MPD by appending the top-level
  `CustomParts/*.dat` definitions, removing UTF-8 BOM markers, and adding the
  standard `!LDRAW_ORG Unofficial_Part` header to custom-part definitions.
- Fall back to `model2.ldr` only when `model.ldr` is absent.
- Resolve project-local `CustomParts/` content so referenced custom parts are
  available while the package is open; no global custom-part installation is
  required.
- Prefer a self-contained embedded model when available and fall back to the
  primary model plus extracted custom-part files when it is not.
- Report unsupported, corrupt, wrong-password, and missing-model packages with
  an actionable error instead of opening an empty or partial model.
- Preserve the original `.io` file and clean project-cache artifacts when they
  are no longer needed.

## Capabilities

### New Capabilities

- `studio-io-import`: Open Studio `.io`/`.mo` project packages and resolve the
  primary model plus project-local custom parts so the complete build renders.

### Modified Capabilities

- None.

## Impact

- File-open UI, file drag-and-drop handling, and recent-file behavior.
- A new Studio package import/normalization service.
- Temporary/project cache management for extracted models and custom parts.
- Renderer search-path configuration for project-local custom parts.
- Tests and fixtures for unencrypted and password-protected Studio packages.

## Non-goals

- This change does not implement Studio `PE_TEX_INFO` texture projection.
- This change does not change decal transparency, depth ordering, alpha testing,
  or transparent-part sorting.
- This change does not install custom parts into the global LDraw library.
- This change does not modify or re-save the source `.io` archive.
- This change does not add general-purpose support for arbitrary Studio custom
  meshes beyond the model files packaged in the `.io`.

## Success Criteria

- Opening the provided `透明件遮挡.io` displays both custom-part instances and
  the complete build geometry without installing custom parts globally.
- A password-protected `.io` equivalent opens successfully by using the Studio
  default archive password.
- The visible part count matches the complete model contained by the package;
  an intentionally incomplete `.ldr` remains unsupported and produces a clear
  missing-part message.
- Opening, closing, and reopening the package does not modify the source archive
  and does not leave stale cache state that changes the displayed model.
