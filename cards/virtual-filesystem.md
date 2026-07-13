# virtual-filesystem

The browser-local project file tree: file types, adding/creating files, and path resolution.

The virtual filesystem is the core of the project structure — a browser-local tree of
files the assembler resolves by project path. Paths use forward slashes, e.g.
`assets/chars/title.chr`, `assets/blocks/tiles.blk`, `assets/maps/level1.map`,
`assets/music/song.w64sid`, `includes/common.asm`, `symbols/kernal.sym`. The virtual path
is what `.include`, `.import`, and `.incbin` use; it need not match any host location.

## Why it exists

Browsers cannot freely read arbitrary host paths. Never write
`.include "C:/Users/Somebody/project/includes/defs.asm"` — add the file to the virtual
tree and write `.include "includes/defs.asm"`. This keeps `.web64proj` portable.

## File types

| Extension | Type | Notes |
|---|---|---|
| `.asm`, `.s` | Source | Assembly source |
| `.c`, `.h` | Source | Web64 C modules and headers |
| `.inc`, `.txt`, `.mac` | Source | Include files, macro files, text |
| `.sym` | Symbols/source | Parsed as symbol source |
| `.chr`, `.ch8` | Charset asset | Editable in Char Editor |
| `.spr` | Sprite bank asset | Editable in Sprite Editor, 64 bytes per frame; plus logical overlay-pair descriptors (`.spritepair.json`) |
| `.blk`, `.blocks` | Block set asset | Editable in Block Editor |
| `.map`, `.w64map` | Map asset | Editable in Map Editor |
| `.sid` | SID binary | Inspectable/previewable in SID Editor |
| `.w64sid` | Web64 SID tracker | Source-truth tracker JSON, editable in SID Tracker |
| `.bin`, `.raw`, `.dat` | Binary | Usable with `.incbin` / `.import binary` |
| `.prg`, `.seq`, `.scr`, `.koa` | Binary | Stored as project binary records |
| `.web64proj`, `.web64project`, `.json` | Project | Full Web64 IDE project |
| `.vsf` | Snapshot | Runtime/emulator state |

Other source-like extensions also accepted: `.def`, `.cfg`.

## Adding files (Files panel)

- New: create a virtual file in the project tree.
- Add files: import one or more browser-selected files.
- Add folder: import a folder tree (when the browser supports directory selection).
- Remove: remove the selected virtual file.
- Clear: remove all virtual import files.

Added `.chr`/`.ch8`/`.spr`/`.blk`/`.blocks`/`.map`/`.w64map`/`.sid`/`.w64sid` files are
converted into the matching editable/inspectable asset records where possible, with
generated include records for supported asset types. Text-like files become source
records; other files become binary records.

## Creating files with New

The extension determines the initial type: `.chr`/`.ch8` → charset asset; `.spr` → sprite
bank asset; `.blk`/`.blocks` → block-set asset; `.map`/`.w64map` → map asset;
`.w64sid` → SID tracker source; `.sid` → SID binary record (SID Editor); `.sym` →
symbols/source file; text-like → editable source; anything else → empty binary record.

## Path resolution order

When resolving an import path, the IDE tries:

1. The path relative to the current source file.
2. The direct normalized path.
3. The same path without leading parent segments.
4. A matching suffix in the virtual tree.
5. A matching base name, when unambiguous.

If a file cannot be found, the compiler reports an import diagnostic. If a base name is
ambiguous, use a more specific virtual path.
