## ADDED Requirements

### Requirement: Open Studio project packages from all file-entry points

myLPub3D SHALL accept `.io` and `.mo` files through the File menu, the generic
Open dialog, the recent-files list, command-line file loading, and drag-and-drop.
All entry points MUST route the package through the same import service.

#### Scenario: Open an IO package from the dedicated action

- **WHEN** the user selects `Open IO File...` and chooses a readable `.io` file
- **THEN** myLPub3D imports the package and loads its selected model

#### Scenario: Open an IO package through the generic Open action

- **WHEN** the user selects `Open...` and chooses a readable `.io` or `.mo` file
- **THEN** myLPub3D routes the file through the Studio package importer

#### Scenario: Open a recent IO package

- **WHEN** the user selects a readable `.io` path from the recent-files list
- **THEN** myLPub3D re-imports the package and restores the logical `.io` path as the current file

### Requirement: Resolve the Studio model and project-local parts

The importer SHALL use `model.ldr` as the top-level model when it is present.
The importer MUST create a temporary self-contained MPD containing the top-level
model, the top-level `CustomParts/*.dat` definitions, BOM-free text, and a
standard `!LDRAW_ORG Unofficial_Part` header for each custom-part definition.
The importer MUST NOT embed files below `CustomParts/p/`. If `model.ldr` is
absent, the importer SHALL use `model2.ldr` as a best-effort fallback.

#### Scenario: Synthesize the myLPub3D-compatible model

- **WHEN** a package contains `model.ldr` and top-level `CustomParts/m28d837e8_2026624_114514.dat`
- **THEN** the importer creates an MPD containing both definitions without a BOM and loads that MPD

#### Scenario: Fall back to the embedded model

- **WHEN** a package has no `model.ldr` and contains `model2.ldr`
- **THEN** the importer loads `model2.ldr` as a best-effort compatibility fallback

#### Scenario: Do not install custom parts globally

- **WHEN** an IO package contains project-local custom parts
- **THEN** the importer uses a per-open cache and does not copy those files into the global LDraw library

### Requirement: Support the Studio archive password convention

The importer SHALL attempt the Studio default archive password `soho0909` when
reading package entries. The password MUST NOT appear in logs, status messages,
error messages, recent-file metadata, or command-line arguments.

#### Scenario: Open an encrypted package

- **WHEN** a Studio package requires the Studio archive password
- **THEN** the importer opens and extracts the package using `soho0909` without prompting the user

#### Scenario: Open an unencrypted package

- **WHEN** a valid Studio package is not encrypted
- **THEN** the importer opens it successfully with the same code path

### Requirement: Show the complete build geometry

After importing the supplied test package, myLPub3D SHALL display every part
instance resolved by the selected model, including instances that reference
project-local `CustomParts/` files. The displayed part count MUST equal the
part count reported by the loaded model.

#### Scenario: Display the supplied package

- **WHEN** the user opens `透明件遮挡.io`
- **THEN** both `m28d837e8_2026624_114514.dat` and `m28d837e8_2026624_115253.dat` instances are resolved and the complete build geometry is displayed

#### Scenario: Compare part counts

- **WHEN** the package model has been loaded
- **THEN** the visible part set and `LDrawFile` part count are consistent with the selected model

### Requirement: Preserve source archives and isolate temporary files

The importer MUST NOT modify the source `.io` or `.mo` file. Extracted files
MUST remain inside the project cache, and the importer MUST reject archive entry
paths that would escape the cache root.

#### Scenario: Source archive remains unchanged

- **WHEN** an IO package is opened, closed, and reopened
- **THEN** its byte contents and modification metadata remain unchanged

#### Scenario: Unsafe archive entry is rejected

- **WHEN** an archive entry resolves outside the project cache root
- **THEN** the importer reports an error and does not write that entry

### Requirement: Report import failures without corrupting the current model

The importer SHALL report unreadable files, invalid ZIP containers, missing
models, extraction failures, and wrong-password archives as user-visible
errors. A failed import MUST leave the previously loaded model and its search
paths unchanged.

#### Scenario: Invalid package

- **WHEN** the user selects a file that is not a valid `.io` or `.mo` ZIP package
- **THEN** myLPub3D reports that the package is invalid and keeps the current model loaded

#### Scenario: Missing model

- **WHEN** a valid archive contains no `modelv2.ldr`, `modelv1.ldr`, `model.ldr`, or `model2.ldr`
- **THEN** myLPub3D reports that the package has no loadable model and keeps the current model loaded

### Requirement: Preserve existing LDraw file behavior

Opening `.ldr`, `.mpd`, and `.dat` files SHALL continue to use the existing
LDraw loader. The IO importer MUST remove its temporary search directories and
cache when the imported model is closed or another file is opened.

#### Scenario: Open an ordinary LDraw file after an IO package

- **WHEN** an IO package is open and the user opens an ordinary `.ldr` file
- **THEN** the IO cache and its search directories are removed before the LDraw file is loaded

#### Scenario: Close an IO package

- **WHEN** the user closes the imported IO package
- **THEN** the project cache is removed and the logical current-file path is cleared
