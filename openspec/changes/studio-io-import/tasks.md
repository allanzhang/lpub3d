Gate summary:

- `proposal.md`: spec-lint round 1 passed with 0 blocker, 0 warning, 0 nit.
- `design.md`: spec-lint round 1 passed with 0 blocker, 0 warning, 0 nit.
- `specs/studio-io-import/spec.md`: spec-lint round 1 passed with 0 blocker, 0 warning, 0 nit; `openspec validate --strict` passed.

## 1. Importer Core

- [x] 1.1 Add a `StudioIoImporter` class and register its source/header in `mainApp/mainApp.pro`; completion is proven by a clean qmake-generated project that includes both files.
- [x] 1.2 Implement ZIP open and entry enumeration using QuaZip with the default password constant `soho0909`; completion is proven by listing `model.ldr` and `CustomParts/*` from the supplied fixture.
- [x] 1.3 Synthesize a BOM-free self-contained MPD from `model.ldr` plus top-level `CustomParts/*.dat`, with `model2.ldr` used only when `model.ldr` is absent; completion is proven by resolving the supplied fixture and loading both custom parts.
- [x] 1.4 Write the synthesized model into a per-open temporary cache while rejecting path-traversal entries; completion is proven by a cache inspection showing the generated MPD and no files outside the cache root.
- [x] 1.5 Return the normalized model path, logical source path, and cache path from one import result; completion is proven by importing the supplied fixture and the encrypted equivalent.

## 2. Password and Failure Handling

- [x] 2.1 Keep the default password in one named constant and ensure it is absent from logs, status messages, errors, recent-file data, and command lines; completion is proven by source review and a log inspection during an encrypted import.
- [ ] 2.2 Return distinct import failures for unreadable files, invalid ZIP containers, missing models, extraction failures, and password failures; completion is proven by unit-level or CLI-level checks for each failure class.
- [ ] 2.3 Keep the current model and search paths unchanged when an import fails; completion is proven by opening a valid model, attempting an invalid IO import, and confirming the valid model remains loaded.

## 3. GUI and File Routing

- [x] 3.1 Add an `Open IO File...` File-menu action with an `*.io *.mo` filter and route it to the importer; completion is proven by the compiled action being registered in the File menu.
- [x] 3.2 Include `.io` and `.mo` in the generic Open dialog and drag-and-drop path, routing both through the same importer; completion is proven by source routing and the command-line IO load path.
- [ ] 3.3 Route recent-file, command-line, reload, and watcher paths through the importer when the logical file ends in `.io` or `.mo`; completion is proven by opening the supplied fixture from Recent Files and reloading it after an archive change.
- [x] 3.4 Keep the `.io`/`.mo` path as the logical current file while loading the synthesized model internally; completion is proven by the log reporting `透明件遮挡.io` while parsing `studio-io-import.ldr`.

## 4. Runtime Lifecycle

- [ ] 4.1 Track project-local resources added by the importer and remove them before loading another file or closing the current model; completion is proven by opening IO followed by LDraw and confirming no IO cache directory remains in `Preferences::ldSearchDirs`.
- [ ] 4.2 Delete the previous project cache after a successful replacement import and delete the new cache after a failed import; completion is proven by inspecting the temporary cache directory after each transition.
- [ ] 4.3 Ensure the ordinary `.ldr`, `.mpd`, and `.dat` loader path is unchanged when the selected file is not `.io` or `.mo`; completion is proven by opening an ordinary LDraw file after the feature build.

## 5. Verification

- [x] 5.1 Build the macOS application with the project's existing qmake/make flow; completion is proven by a successful build exit code and an updated application binary.
- [ ] 5.2 Open `透明件遮挡.io` and verify both custom-part instances and the complete build are visible in the UI; completion is proven by a screenshot or UI observation from the user-facing verification run.
- [x] 5.3 Verify that no global custom-part installation is required; completion is proven by opening the fixture with only its per-open cache and no added files under the global LDraw custom-part directory.
- [x] 5.4 Prove the source archive remains byte-identical after open, close, and reopen; completion is proven by the passing `preservesSourceArchive` test and matching SHA-256 hashes before and after.
- [x] 5.5 Verify an encrypted IO equivalent opens with `soho0909` without a password prompt; completion is proven by the passing `importsPasswordProtectedPackage` test and a log without the password value.

## 6. OpenSpec Closeout

- [x] 6.1 Run `spec-lint` against the final proposal, design, and spec and record that all blocker counts are zero; completion is proven by the saved lint outputs.
- [x] 6.2 Run `openspec validate --strict studio-io-import` and confirm the change is valid; completion is proven by the command output.
- [x] 6.3 Record the exact test fixtures, commands, observed part count, archive hash, and known limitation that PE_TEX_INFO remains deferred to phase two; completion is proven by the final verification note.
