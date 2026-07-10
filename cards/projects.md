# projects

How Web64 IDE projects are saved, opened, and what a `.web64proj` file contains.

Projects are saved as `.web64proj` files — JSON with enough information to recreate the
browser-local project state on any machine.

## What a project stores

- Main source text and source file name.
- Assembly origin and selected entry label.
- Virtual project files.
- Binary assets, stored as base64.
- Generated include files.
- Graphics asset metadata: mode, dimensions, color slots, include paths, counts.
- IDE emulator settings: display scale, audio, SID quality, scanlines, drive 9, gamepad,
  joystick port, warp, live patch setting.

## What a project does NOT store

- Native host filesystem handles as required state.
- Browser runtime memory.
- Unsaved emulator snapshot state (export separately as `.vsf` if needed).
- External source tree paths outside the virtual project.

## Opening a project

Use the project open button in the top toolbar. If the browser supports the File System
Access API, the IDE opens a file handle directly; otherwise it uses a standard file input
fallback. Opening a project replaces the current source, virtual file tree, selected entry
label, and persisted emulator settings. Current runtime state is cleared from the IDE model.

## Saving a project

Use the project save button. The IDE writes the `.web64proj` through the browser file
picker when available, otherwise it downloads the file. Before saving, the IDE flushes
pending character and sprite draft edits into the virtual filesystem, so the saved project
contains the bytes currently visible in the asset editors, not stale bytes.

## Saving source and PRG files separately

- Source save: saves only the currently edited source text.
- PRG save: compiles the current source and virtual files, then saves the generated PRG.

Use project save to preserve the entire IDE state; use source/PRG save when you only want
a single artifact.
