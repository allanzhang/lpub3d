## Why

Studio stores custom-part sticker projections in `PE_TEX_INFO` instead of LDraw
`TEXMAP`. An earlier implementation synthesized decal-only triangles. Decompiled
Studio behavior and rendered evidence falsified that architecture: Studio writes
UV coordinates back to the original flattened mesh and does not create sticker
geometry. The instruction designer also needs opaque sticker pixels to hide
transparent internal geometry while transparent geometry in front remains
visible.

## What Changes

- Parse Studio `PE_TEX_INFO` inside the IO import path and decode the embedded
  PNG into the per-open project cache.
- Wrap the custom part's existing base-part reference in an internal Studio
  texture directive without replacing the reference or adding sticker primitives.
- Preserve the original triangle set while reproducing Studio's planar-UV
  selection, connected-triangle expansion, inverse-matrix, box-extents, and UV
  assignment rules.
- Export textured geometry to POV-Ray as `mesh2` with `uv_vectors` and an
  `image_map`, so PNG alpha controls transparent pixels and opaque sticker pixels
  participate in normal depth/occlusion behavior.
- Keep Native rendering as a preview path only; POV-Ray output and human visual
  inspection are the visual acceptance target.
- Preserve the internal Studio texture directive when LPub3D prepares CSI/export
  models, without writing that directive into ordinary saved LDraw files.
- Add regression coverage for the supplied `透明件遮挡.io` custom parts using
  `6112.dat` and `37352.dat`.

## Capabilities

### New Capabilities

- `studio-decal-rendering`: Apply Studio `PE_TEX_INFO` UV mapping to the original
  custom-part mesh and export alpha-aware textured geometry through POV-Ray.

### Modified Capabilities

- None.

## Impact

- Studio IO importer and project-cache texture extraction.
- LDraw mesh loading for project-local custom parts.
- POV-Ray mesh export for textured sections.
- UV parity and IO regression tests.
- Manual visual comparison rendered through POV-Ray.

## Non-goals

- This change does not generate decal-only triangles, quads, or sticker meshes.
- This change does not apply Studio textures globally to cached base parts such
  as `6112.dat` or `37352.dat`.
- This change does not introduce a Native renderer decal pass or claim Native
  transparency parity.
- This change does not add LDGLite, external LDView, or arbitrary custom-mesh
  support.
- This change does not alter or re-save the source `.io` archive.

## Success Criteria

- Importing `透明件遮挡.io` retains both original custom-part base references and
  does not add any `3 15` or `3 16` sticker triangles.
- The `6112.dat` and `37352.dat` triangle sets remain unchanged after UV mapping;
  selected original triangles receive UV coordinates within `1e-4` of the Studio
  algorithm.
- The generated POV-Ray scene contains `mesh2`, `uv_vectors`, and `image_map`
  entries for both decoded stickers.
- Opaque sticker pixels hide transparent internal geometry in POV-Ray, while
  transparent PNG pixels do not write opaque sticker color or depth.
- Reopening the same package produces identical texture hashes and deterministic
  UV/POV output.
- Existing geometry-only IO import tests remain green.
