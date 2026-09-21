## ADDED Requirements

### Requirement: Decode Studio PE_TEX_INFO data

The IO importer SHALL parse `PE_TEX_INFO` records from top-level Studio custom
parts. It MUST decode the embedded Base64 image into the project cache, using a
deterministic file name derived from the source texture bytes.

#### Scenario: Decode the supplied stickers

- **WHEN** `透明件遮挡.io` is imported
- **THEN** both embedded `PE_TEX_INFO` images are decoded into the project cache and the generated model references those images

#### Scenario: Re-import preserves texture identity

- **WHEN** the same IO package is imported twice
- **THEN** each decoded texture has the same SHA-256 hash on both imports

### Requirement: Preserve original custom-part geometry

The importer MUST keep each texture-bearing custom part's original base-part
reference. It MUST NOT replace that reference with generated vertices, quads, or
sticker triangles.

#### Scenario: Base-part references remain

- **WHEN** the custom parts containing `6112.dat` and `37352.dat` are imported
- **THEN** both original type-1 references remain in the generated model

#### Scenario: No sticker geometry is emitted

- **WHEN** the generated model for `透明件遮挡.io` is inspected
- **THEN** it contains no new `3 15` or `3 16` sticker primitives

### Requirement: Apply Studio planar UVs to original triangles

The mesh processor SHALL reproduce Studio's planar projection selection over the
original triangle list. Seed selection MUST use the inverse projection matrix,
signed box extents, the Studio down normal, the unassigned-UV guard, and
triangle-box intersection. Selection MUST then expand to triangles connected by
shared vertex indices. The processor MUST assign UVs from transformed position
components and MUST NOT change triangle winding, position, or count.

#### Scenario: 1x12 UV parity

- **WHEN** the `6112.dat` base mesh is processed with its Studio texture record
- **THEN** the selected triangle indices and UV coordinates match the Studio algorithm within `1e-4`

#### Scenario: Curved 1x2 UV parity

- **WHEN** the `37352.dat` base mesh is processed with its Studio texture record
- **THEN** the selected triangle indices and UV coordinates match the Studio algorithm within `1e-4`

#### Scenario: Geometry is preserved under UV mapping

- **WHEN** the processor maps the supplied `6112.dat` and `37352.dat` meshes
- **THEN** each mesh has the same triangle count and vertex positions before and after mapping

### Requirement: Keep Studio texture scoped to the custom part

The generated texture directive MUST be consumed while loading the custom part
that contains it. It MUST NOT register or mutate the globally cached `6112.dat`
or `37352.dat` base part.

#### Scenario: Other instances are unaffected

- **WHEN** the IO package is open and another model reuses `6112.dat` without a Studio texture directive
- **THEN** that base part remains untextured

### Requirement: Preserve the Studio texture directive through CSI preparation

The CSI/export preparation path SHALL carry valid `!STUDIO_TEXMAP START/END`
directives into the model consumed by the mesh loader. The directive MUST remain
associated with the custom part that contained it, and ordinary LDraw save
operations MUST NOT add the internal directive to non-IO models.

#### Scenario: CSI retains the texture directive

- **WHEN** a page containing an IO Studio custom part is prepared for rendering or POV export
- **THEN** the generated CSI model contains the matching `!STUDIO_TEXMAP START/END` pair

#### Scenario: Normal save does not leak the directive

- **WHEN** a non-IO LDraw model is saved normally
- **THEN** no `!STUDIO_TEXMAP` directive is written

### Requirement: Export alpha-aware POV textures

POV-Ray export SHALL emit each textured section as `mesh2` geometry with
`vertex_vectors`, `normal_vectors`, `uv_vectors`, `face_indices`, and an
`image_map` texture for the decoded PNG. PNG alpha MUST control transparent
sticker pixels so opaque pixels participate in depth and occlusion while
transparent pixels do not render as opaque sticker color.

#### Scenario: POV contains both textured meshes

- **WHEN** the supplied IO package is exported to POV-Ray
- **THEN** the scene contains `mesh2`, `uv_vectors`, and `image_map` entries for both decoded stickers

#### Scenario: Opaque sticker pixels occlude internal geometry

- **WHEN** the supplied model is rendered through POV-Ray
- **THEN** opaque sticker pixels hide transparent-part studs and internal contours behind them

#### Scenario: Transparent texture pixels remain transparent

- **WHEN** the PNG contains zero-alpha background pixels
- **THEN** those pixels do not write opaque sticker color or depth and the transparent hull remains visible

### Requirement: Preserve geometry-only import on decal failure

If a `PE_TEX_INFO` record is malformed or its image cannot be decoded, the
importer MUST retain the geometry import and report a decal-specific warning.

#### Scenario: Malformed texture does not lose geometry

- **WHEN** one custom part has an invalid `PE_TEX_INFO` record
- **THEN** the base part geometry remains in the imported model and the importer reports the texture failure

### Requirement: Keep existing IO and LDraw behavior intact

The Studio texture conversion SHALL run only for IO custom parts containing a
valid `PE_TEX_INFO` record. Existing geometry-only IO imports, `.ldr`, `.mpd`,
and `.dat` loading MUST remain unchanged.

#### Scenario: Geometry-only package still loads

- **WHEN** an IO package contains no valid `PE_TEX_INFO` records
- **THEN** the synthesized MPD loads with the same geometry behavior as before this change
