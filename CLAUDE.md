Write programs for C64

IDE     https://web64.nofs.ai/ide/
Manual  https://web64.nofs.ai/docs/web64-ide-user-manual.html
        (local copy: `web64-ide-user-manual.html` — source of the cards below)

`README.md` has the human-facing overview: project table, IDE workflow, repo layout.
Keep it in sync when projects are added or renamed.

Each project's `<name>.web64proj` is GENERATED — never edit it by hand. It is built
from the sources listed in the project's `web64proj.json` manifest: `make proj`
rebuilds all (deterministic output), `make check` verifies sync, `make serve` starts
the CORS server for the IDE automation workflow, `make manual` refreshes the manual.

## Projects

Source code lives under `projects/<name>/`, one folder per web64 project — the `.asm`
sources plus any `.web64proj`, `.chr`, `.spr`, or PRG artifacts that belong to it.
Create new projects as new folders here. **Each project has its own `CLAUDE.md`** with
file layout, memory map, conventions, and testing notes — read it before working there.

- `projects/breakout/` — Breakout game, single file; sprite paddle/ball, char bricks,
  joystick port 2 or A/D/SPACE keyboard (KERNAL current-key at `$cb`)
- `projects/fixmath/` — signed 8.8 fixed-point math + hires gfx library; `lib/` here is
  the single source of truth (raytracer symlinks it)
- `projects/raytracer/` — mirror sphere over a checkered floor: shadows, sky gradient,
  animation-stable dithering (Bayer / white noise / blue noise)
- `projects/raytracer-c/` — same image, fully ported to Web64 C v0.1: all executable
  code is C (main/trace/fixmath/gfx/scene as C modules; arithmetic in inline-asm
  blocks, branching as C ifs on asm-set flags); scene constants are runtime C
  globals; no assembly files at all — equates and data live in asm() blocks. Standalone.

## Cards

Lazy-loaded reference cards split from the Web64 IDE user manual. Open a card when its
trigger matches the task.

### Overview
- [ide-overview](cards/ide-overview.md) — first orientation, workspace layout, what the IDE is and its principles

### Domains
- [projects](cards/projects.md) — opening/saving projects, what `.web64proj` contains, saving source or PRG artifacts
- [virtual-filesystem](cards/virtual-filesystem.md) — adding/creating project files, file extensions, import path resolution, "include file not found"
- [assembler](cards/assembler.md) — writing 6502 assembly: labels, numbers, expressions, data/origin directives, instructions, entry point, compile diagnostics
- [c-compiler](cards/c-compiler.md) — Web64 C v0.1: supported C subset, `c64.h` and bundled headers, config, `asm()` inline assembly, mixed C/assembly projects
- [includes-and-binary-data](cards/includes-and-binary-data.md) — `.include`/`.import`/`.incbin`, symbol files, embedding binaries under a label, splitting source across files
- [macros](cards/macros.md) — defining/invoking macros, parameter substitution, macro-local labels, recursion limits
- [build-and-run](cards/build-and-run.md) — compiling to PRG, Load/Start PRG, SYS entry, runtime start/pause/stop/reset/warp controls
- [runtime-and-input](cards/runtime-and-input.md) — emulator display/audio/SID settings, drive 9, keyboard focus, joystick and gamepad input
- [debugger](cards/debugger.md) — breakpoints, pausing, registers, memory dumps, the line map
- [live-patching](cards/live-patching.md) — patching code/asset changes into the running emulator, when patches don't apply, when to reload instead
- [char-editor](cards/char-editor.md) — creating/editing character sets, mono vs multicolor chars, adding a charset to a program
- [sprite-editor](cards/sprite-editor.md) — creating/editing sprite banks, 64-byte frames, overlay pairs (`_mc`/`_ol`), animation preview, adding sprites to a program
- [block-editor](cards/block-editor.md) — creating/editing tile block sets (`.blk`/`.blocks`) from a charset, charset → block set → map workflow
- [map-editor](cards/map-editor.md) — creating/editing maps (`.map`/`.w64map`) of block indices, map tools, exporting map bytes for assembly
- [sid-tracker](cards/sid-tracker.md) — composing `.w64sid` songs: patterns, instruments, tables, driver limits, generated `.sid`/`.bin`/`.inc` outputs
- [sid-editor](cards/sid-editor.md) — inspecting/previewing imported PSID/RSID `.sid` binaries, metadata, include generation
- [generated-includes](cards/generated-includes.md) — labels/constants generated from `.chr`/`.spr`/`.blk`/`.map`/SID assets, sprite/charset color constants, minimal complete example
- [snapshots](cards/snapshots.md) — quick save/load, `.vsf` snapshot import/export

### Troubleshooting
- [troubleshooting](cards/troubleshooting.md) — anything broken: black page, garbage data, patch not visible, READY prompt, breakpoint or gamepad not working
