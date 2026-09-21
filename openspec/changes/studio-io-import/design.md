## Context

Studio 2.0 `.io` and `.mo` files are ZIP project packages. The installed
Studio implementation opens the archive, selects `modelv2.ldr`, `modelv1.ldr`,
or `model.ldr`, and loads project-local files under `CustomParts/`. A model
opened outside Studio therefore needs both the selected model and its
project-local custom-part files.

myLPub3D currently supports only LDraw text files and has no package import
layer. Opening an extracted `.ldr` loses the custom-part definitions and cannot
display the complete build. The existing file-open, recent-file, watcher,
renderer, and cache paths all assume that the path passed to `openFile()` is an
LDraw file.

## Goals / Non-Goals

**Goals:**

- Open `.io` and `.mo` files from the File menu, generic Open dialog, recent
  files, and drag-and-drop.
- Resolve the primary model and project-local custom parts using the Studio
  package structure.
- Display the complete build geometry for the supplied test package without
  installing custom parts globally.
- Support encrypted Studio archives with the default Studio archive password.
- Keep source archives read-only and isolate extracted files in a temporary
  project cache.
- Preserve the ordinary `.ldr`, `.mpd`, and `.dat` workflows.

**Non-Goals:**

- Do not parse or render Studio `PE_TEX_INFO` textures in this change.
- Do not alter transparent/decal depth ordering or alpha behavior.
- Do not add a general custom mesh importer.
- Do not persist extracted Studio project files into the user's global LDraw
  library.

## Decisions

### Decision: Add a dedicated Studio package importer

Introduce a small importer service that owns ZIP reading, model selection,
safe extraction, and the lifetime of the temporary project directory. The GUI
file-open code calls this service and then loads the normalized model path.

Alternative considered: add ZIP handling directly inside `LDrawFile`. Rejected
because `LDrawFile` is a parser/model container, not a project package manager;
embedding ZIP and cache lifecycle logic there would couple unrelated concerns.

### Decision: Synthesize a myLPub3D-compatible self-contained MPD

Use the package's standard `model.ldr` as the top-level model. Create a temporary
MPD by appending the top-level `CustomParts/*.dat` definitions, removing each
file's UTF-8 BOM, and inserting `0 !LDRAW_ORG Unofficial_Part` after the
`0 Name:` line of custom-part wrappers. Do not embed `CustomParts/p/**`; those
primitive definitions are resolved from the installed LDraw library.

This avoids three incompatibilities found during end-to-end testing:

- `modelv2.ldr` uses Studio's extended type `11` line format, which myLPub3D
  does not parse as a standard LDraw part reference.
- Studio's generated `model2.ldr` rewrites embedded geometry to color `-1`,
  which myLPub3D treats as an unresolved color.
- Embedded primitive paths such as `48/1-4edge.dat` are flattened by the
  Native renderer's temporary-file writer, so embedding them as MPD subfiles
  causes missing-file errors.

If `model.ldr` is absent, use `model2.ldr` only as a best-effort fallback and
report that the package is using a compatibility path.

Alternative considered: use `modelv2.ldr` plus external `CustomParts/` as the
primary path. Rejected because the extended line format and incomplete external
part registration produced zero resolved top-level parts.

### Decision: Use the default Studio archive password automatically

Set the constant Studio archive password `soho0909` on the ZIP reader before
opening entries. The password is tried automatically for both encrypted and
unencrypted packages. It must never be written to logs, status text, error
messages, recent-file metadata, or test output.

Alternative considered: prompt for a password on every encrypted archive.
Rejected because the user explicitly requested automatic default-password
handling and Studio uses the same archive convention.

Alternative considered: store the password in user preferences. Rejected for
this change because there is no user-supplied secret or rotation workflow to
justify persistent configuration.

### Decision: Keep extracted files in a per-open temporary cache

Extract only model files and `CustomParts/` entries, reject paths that escape
the cache root, and retain the cache while the imported model is open. Delete
the previous cache when a new file is successfully opened or when the model is
closed. On failure, delete the newly created cache and leave the currently
loaded model untouched.

Alternative considered: overwrite a fixed `studio-io` directory. Rejected
because concurrent or failed imports could mix artifacts from different
archives.

Alternative considered: load entries directly from memory. Rejected because
the existing LDraw loader, watcher, renderer search paths, and part resolver
require filesystem paths.

### Decision: Track the logical source file separately from the load path

The GUI continues to use the `.io`/`.mo` path for window title, recent files,
watcher, reload, and user-facing messages, while `LDrawFile` loads the extracted
model path. Closing the logical file also releases the temporary cache and
removes its search directories.

Alternative considered: expose the extracted path as the current file.
Rejected because it would corrupt recent-file behavior, make reload target a
generated file, and expose internal cache paths to users.

### Decision: Add both dedicated and general open entry points

Add an `Open IO File...` File-menu action with an `.io`/`.mo` filter, and add
the same extensions to the generic Open dialog and drag-and-drop acceptance.
All entry points route through the same importer.

Alternative considered: only add a dedicated action. Rejected because recent
files, command-line loading, and drag-and-drop would otherwise behave
inconsistently.

## Risks / Trade-offs

- [Large Studio packages can increase import time and disk usage] → Extract only
  model files and `CustomParts/`, show the normal loading status, and release
  the cache when the file closes.
- [ZIP entry names can attempt path traversal] → Normalize and validate every
  output path against the cache root; skip unsafe entries and report an import
  error.
- [A password-protected archive can fail if the Studio convention changes] →
  keep the password in one named constant, return a specific wrong-password or
  corrupt-archive error, and avoid partially loading the model.
- [Project-local search directories can leak into later files] → track the
  directories added by the importer and remove them before loading another
  model or closing the current one.
- [The normal Studio model plus custom parts may not resolve identically in
  lclib and LDView] → prefer the self-contained `model2.ldr` fallback when
  normal resolution fails, and verify the supplied package through both the
  model parser and renderer.

## Migration Plan

1. Add the importer and its package-resolution tests without changing existing
   LDraw file behavior.
2. Route `.io`/`.mo` paths through the importer while keeping existing
   `.ldr`/`.mpd`/`.dat` handling unchanged.
3. Add the UI action and include the new extensions in generic open and
   drag-and-drop paths.
4. Verify the supplied package, an unencrypted archive, and an encrypted
   equivalent before release.
5. Rollback is limited to removing the new action and returning `.io`/`.mo`
   paths to the unsupported-file error; no existing file format is migrated.

## Open Questions

None for implementation. The default password and phase boundary are fixed by
the approved proposal.
