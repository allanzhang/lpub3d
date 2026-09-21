# Studio Decal Rendering Verification

## Objective Precheck (Agent)

### Passed

- OpenSpec structural validation:
  - `openspec validate --strict studio-decal-rendering`
  - Result: `Change 'studio-decal-rendering' is valid`
- Spec-lint round 1:
  - `proposal.md`: 0 blocker, 0 warning, 0 nit
  - `design.md`: 0 blocker, 0 warning, 0 nit
  - `specs/studio-decal-rendering/spec.md`: 0 blocker, 0 warning, 0 nit
- IO/import and processor tests:
  - `STUDIO_IO_FIXTURE='/Users/allan/Documents/Work/IO Enhancement/透明件遮挡.io' ./studio_io_import_test`
  - Result: 10 passed, 0 failed, 1 skipped
- Full application build:
  - `make -j4`
  - Result: exit code 0
- Generated CSI metadata:
  - `csi.ldr` contains two `!STUDIO_TEXMAP START/END` pairs paired with `6112.dat` and `37352.dat`.
- Final POV scene:
  - `/Users/allan/Documents/Work/IO Enhancement/透明件遮挡-export.pov`
  - 2 `mesh2` blocks
  - 2 `uv_vectors` blocks
  - 2 `image_map` blocks with valid decoded PNG paths
  - 2 `face_indices` blocks
- POV-Ray render:
  - Exit code: 0
  - Output: `/tmp/studio-pov-final.png`
  - Resolution: 1600 x 1200
  - Mechanical image check: 534 pixels with saturation above 20

## Visual Acceptance (Human)

Status: **待人工验收**

The agent has completed the mechanical precheck only. The user must inspect the
rendered PNG for:

- sticker position on both custom parts
- sticker orientation and scale
- transparent texture background
- opaque sticker occlusion of internal transparent geometry
- transparent hull behavior in front of and behind the sticker

## Limitations

- Native preview is not the visual acceptance target and does not claim Studio
  shader or transparency parity.
- LDGLite and external `/Applications/LDView.app` are not part of this change.
- POV-Ray is the final rendering path for the Studio texture asset.
