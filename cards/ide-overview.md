# ide-overview

What Web64 IDE is, its guiding principles, and how the workspace is laid out.

Web64 IDE (https://web64.nofs.ai/ide/) is a browser-native Commodore 64 development
environment built around the Web64 VICE port. It combines an assembly editor, a virtual
project filesystem, a 6502 assembler, an experimental C compiler (Web64 C), graphics,
tile-map, and SID music asset editors, an embedded C64 runtime, live patching, and
debugging tools in one web application. It is designed for short
feedback loops: edit source or assets, build a PRG, load it into the embedded emulator,
run, inspect, repeat — all without leaving the browser.

The IDE is separate from the root Web64 emulator page. The root emulator is a lightweight
surface for running existing C64 media; the IDE is the development surface with editors,
project state, diagnostics, and debug metadata.

## Principles

- Browser first: no Electron, Tauri, native daemon, or direct host filesystem access.
- Local by default: project files, assets, source, and generated PRG data live in browser
  memory until explicitly saved or downloaded.
- Virtual filesystem as source of truth: imported files live in the project tree; assembly
  refers to project paths, never machine-specific host paths (`C:\...`, `/home/...`).
- Reproducible projects: a `.web64proj` file contains everything needed to reopen the work.
- Emulator separation: IDE-only features belong under `/ide`.
- VICE compatibility: runtime behavior comes from the VICE C64 core.
- Fast edit-run loop: change source or assets, compile, load, start, inspect, repeat.
- Asset-aware assembly: char/sprite assets are ordinary virtual binary files included with
  `.import binary` or `.incbin`; generated include files expose useful labels.
- Graceful degradation: fall back to download-based saving or show diagnostics rather than
  lose project data.

## Workspace layout

- Top toolbar: project and build actions (open/save project, load/save source, save PRG,
  load PRG, start PRG) and Help / Documentation (opens the user manual; also available
  as Markdown and PDF in `public/docs`).
- Left project sidebar: project source, origin/start controls, entry point selection,
  project status, metrics, and symbols.
- File tree: virtual project files used by `.include`, `.import`, `.incbin`, asset
  editors, and generated outputs.
- Center workbench: tabbed views — Code, Debugger, Char Editor, Sprite Editor,
  Block Editor, Map Editor, SID Tracker, SID Editor. Tab groups: Workspace (Code,
  Debugger) and Assets (the six asset editors).
- Embedded emulator panel: the Web64 runtime surface, shown inside the Code tab.
- Inspector sidebar: compiler diagnostics, line map, byte output, targets, trace
  highlights, and breakpoint toggles.

The Code tab is split between the editor and the embedded emulator. Splits and sidebars
are resizable; sidebars can be collapsed to preserve workspace area.
