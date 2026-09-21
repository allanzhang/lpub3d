## Context

Studio custom parts contain `PE_TEX_INFO` records with sixteen floating-point
projection parameters, UV bounds, and a Base64 PNG. Decompiled Studio methods
show the reference behavior:

- `InitWithLine` maps the first twelve values into a `Matrix4x4`.
- `CalculateInverseMatrixAndBoxExtents` combines the mesh parent matrix with
  that matrix, extracts translation, rotation, and signed scale, and builds the
  inverse projection transform.
- `CollectTrianglesInBoxExtents` selects seed triangles only when all three
  vertices are still unassigned (`UV.x <= 0`), the transformed normal faces
  Studio's down vector within `0.001`, and the triangle intersects the extents
  box.
- `CollectConnectedTriangles` expands selection across triangles sharing any
  vertex index.
- `AssignAtlasCoordsFromPlanarProjection` computes UV from the transformed
  position and writes it back to the original `VertexList`; it does not add
  triangles.

myLPub3D imports the IO custom parts but does not understand this proprietary
record. The existing mesh pipeline already supports textured triangle sections,
but the implementation must keep the texture attached to the custom part rather
than to the globally cached base part.

## Goals / Non-Goals

**Goals:**

- Preserve each custom part's original base-part reference and triangle set.
- Apply Studio planar UV selection and assignment to a copy of that base mesh
  while loading the custom part.
- Keep the custom part self-contained and deterministic.
- Export textured sections as POV-Ray `mesh2` with `uv_vectors` and `image_map`.
- Make POV-Ray responsible for alpha-controlled transparency and depth.
- Prove UV calculations and structural invariants with automated tests.

**Non-Goals:**

- No sticker-only geometry, quads, or generated triangles.
- No global mutation of `6112.dat`, `37352.dat`, or any other base-part cache.
- No Native decal pass or claim of Native visual parity.
- No exact Unity shader reproduction.

## Decisions

### Decision: Attach Studio texture metadata to the custom part, not the base part

The importer SHALL retain the existing `1 ... 6112.dat` / `1 ... 37352.dat`
reference and add an internal `!STUDIO_TEXMAP START/END` block around it. The
marker carries the sixteen values and the cache-relative texture path. The mesh
loader consumes the marker while loading only that custom part.

Alternative considered: register the texture against the base-part filename in
the global piece library. Rejected because piece caches are shared; opening the
IO would texture every other use of the same base part and would make the result
non-local.

### Decision: Apply UVs to copied original triangles

When the loader expands the wrapped base-part reference into the custom part's
`lcLibraryMeshData`, it records the section ranges created by that reference.
Only those new triangle sections are considered. A dedicated
`StudioMeshProcessor` reproduces the Studio seed selection, connected expansion,
and UV calculation. Selected triangles are moved to textured triangle sections
using their original positions, normals, winding, and color. The total triangle
count does not change, and no unselected geometry or base-part cache is altered.

The processor keeps parity fixtures for `6112` and `37352` with a `1e-4`
tolerance. The structural invariant is stronger than a visual check: no sticker
primitive may exist in the generated model and no base-part reference may be
removed.

### Decision: Use POV-Ray for visual transparency semantics

The Native renderer remains a preview. POV-Ray export SHALL emit textured
sections as `mesh2` objects with `uv_vectors` and an `image_map` texture. PNG
alpha determines transparent versus opaque texels, allowing opaque sticker pixels
to write depth and transparent pixels to preserve the transparent hull behind
them. Non-textured sections retain the existing POV export behavior.

Alternative considered: add another OpenGL blend/depth pass. Rejected for this
change because Native ordering and refraction are not the acceptance surface
and would add renderer risk without proving UV correctness.

### Decision: Keep malformed texture records non-fatal

If `PE_TEX_INFO` is malformed or its image cannot be decoded, the importer
preserves the custom part geometry, omits only the texture marker, and records a
decal-specific warning.

### Decision: Preserve the internal directive through CSI preparation

LPub3D rebuilds page content in `Gui::writeToTmp` before rendering or exporting.
That path currently drops unknown meta commands, including the internal
`!STUDIO_TEXMAP` directive. The CSI writer SHALL explicitly pass through valid
Studio texture start/end directives while leaving ordinary LDraw save paths
unchanged. This keeps the marker associated with the custom part in the model
consumed by the mesh loader and POV exporter.

## Risks / Trade-offs

- [Studio computes UVs after flattening and matrix decomposition] → Keep the
  processor independent of the renderer, encode the exact Studio formulas, and
  test vertices, triangle indices, selected indices, and UVs.
- [POV syntax or alpha interpretation can be wrong] → Validate generated POV
  with the bundled Studio 2.0 POV-Ray executable and retain the generated scene
  as a review artifact.
- [Copied triangles duplicate textured vertices in the mesh cache] → Accept
  vertex duplication required by texture coordinates while asserting that the
  triangle count and rendered positions are unchanged.
- [Internal marker is non-standard LDraw] → Emit it only in the IO import cache;
  never write it back to the source `.io` or ordinary user models.

## Migration Plan

1. Replace decal-only generation with the internal Studio texture marker while
   preserving the base-part reference.
2. Add the independent Studio mesh processor and parity tests.
3. Integrate the processor with newly added custom-part sections only.
4. Extend POV export with `mesh2` UV textures.
5. Run IO, UV, structural, POV, and geometry regression tests.
6. Render the supplied fixture and hand the output to the user for visual
   inspection.

## Open Questions

- None for the supplied fixture. Additional Studio texture methods, such as
  UV-mapped or sheared textures, remain outside this change.
