Gate summary:

- `proposal.md`: spec-lint round 1 passed with 0 blocker, 0 warning, 0 nit.
- `design.md`: spec-lint round 1 passed with 0 blocker, 0 warning, 0 nit.
- `specs/studio-decal-rendering/spec.md`: spec-lint round 1 passed with 0 blocker, 0 warning, 0 nit; `openspec validate --strict` passed.

## 1. Import Contract

- [x] 1.1 Keep the original base-part reference and add a scoped `!STUDIO_TEXMAP START/END` block around it; fixture tests prove the generated import contains no `!TEXMAP` replacement.
- [x] 1.2 Decode each `PE_TEX_INFO` PNG into the project cache with a deterministic SHA-256-based name; importer tests cover the supplied fixture and metadata parser.
- [ ] 1.3 Keep malformed texture data non-fatal and emit a warning while preserving geometry; the malformed-record fixture remains outstanding.
- [x] 1.4 Remove the global base-part texture registry path; the lclib registry/post-load patch is no longer in the build.

## 2. Studio Mesh Processor

- [x] 2.1 Add an independent Studio mesh processor with projection matrix construction, signed extents, seed selection, connected expansion, and UV output; unit tests cover seed UV and connectivity.
- [x] 2.2 Implement PE_TEX_INFO matrix construction, signed box extents, and inverse projection transform; unit coverage exercises the identity-projection case.
- [x] 2.3 Implement seed selection with the unassigned-UV guard, down-normal threshold, and triangle-box intersection; unit coverage rejects an outside triangle and accepts an intersecting one.
- [x] 2.4 Implement connected-triangle expansion over shared vertex indices; unit coverage includes expansion beyond the seed triangle.
- [ ] 2.5 Prove `6112.dat` and `37352.dat` selected-index and UV parity against independently derived Studio reference values within `1e-4`.

## 3. Mesh Pipeline Integration

- [x] 3.1 Apply Studio texture metadata only to sections copied from the wrapped custom-part base reference; the lclib adapter operates on recorded section ranges.
- [x] 3.2 Move selected triangles to textured sections while preserving original positions, normals, winding, and color; the adapter reuses existing vertex data.
- [ ] 3.3 Verify deterministic end-to-end mesh loading through the LPub3D page/export pipeline.
- [x] 3.4 Preserve `!STUDIO_TEXMAP START/END` through `Gui::writeToTmp` CSI preparation while keeping normal LDraw save paths unchanged; generated CSI inspection shows two marker pairs.
- [x] 3.5 Apply the Studio mesh processor to the inline MPD/CSI `lcModel` mesh-building path; POV export now contains two textured `mesh2` objects with two `uv_vectors` and two `image_map` blocks and no separate sticker primitives.

## 4. POV Export

- [x] 4.1 Export textured sections as `mesh2` with `vertex_vectors`, `normal_vectors`, `uv_vectors`, and `face_indices`; a focused harness generated `/tmp/pov_mesh_test.pov` with all required blocks.
- [x] 4.2 Attach decoded PNGs through `image_map`; the focused harness generated the expected texture block.
- [x] 4.3 Export the supplied IO fixture with visible sticker pixels; the final POV contains two textured meshes and the rendered PNG has 534 pixels with saturation above 20.

## 5. Verification And Closeout

- [x] 5.1 Run the IO importer and Studio mesh processor tests; 9 passed, 0 failed, 1 skipped (encrypted fixture not supplied).
- [x] 5.2 Build the full application; `make -j4` completed successfully.
- [x] 5.3 Render the supplied fixture through POV-Ray with visible sticker pixels; POV-Ray exits 0 and produces a 1600x1200 PNG.
- [ ] 5.4 Ask the user to inspect sticker position, orientation, transparency, and occlusion; visual acceptance remains pending.
- [x] 5.5 Re-run spec-lint and `openspec validate --strict studio-decal-rendering`; all documents passed with zero unresolved blockers.
- [x] 5.6 Record final limitations for Native preview, LDGLite, and external LDView in the final verification note.

## Current Blocker

The importer preserves `!STUDIO_TEXMAP` in `studio-io-import.ldr`, and lclib can consume it when loading that model directly. However LPub3D's page/export pipeline regenerates `csi.ldr` and strips both `!STUDIO_TEXMAP` and `PE_TEX_INFO` from custom-part definitions. The generated `csi.ldr` contains only the base-part references, so `Project::ExportPOVRay` reports “没有可导出的内容” before any textured mesh can be exported. The next SDD task is to preserve the Studio texture directive through LPub3D's model serialization or export directly from the intact imported MPD.
