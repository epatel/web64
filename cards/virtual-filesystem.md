# virtual-filesystem

The browser-local project file tree: file types, adding/creating files, and path resolution.

The virtual filesystem is the core of the project structure — a browser-local tree of
files the assembler resolves by project path. Paths use forward slashes, e.g.
`assets/chars/title.chr`, `includes/common.asm`, `symbols/kernal.sym`. The virtual path
is what `.include`, `.import`, and `.incbin` use; it need not match any host location.

## Why it exists

Browsers cannot freely read arbitrary host paths. Never write
`.include "C:/Users/Somebody/project/includes/defs.asm"` — add the file to the virtual
tree and write `.include "includes/defs.asm"`. This keeps `.web64proj` portable.

## File types

| Extension | Type | Notes |
|---|---|---|
| `.asm`, `.s` | Source | Assembly source |
| `.inc`, `.txt`, `.mac` | Source | Include files, macro files, text |
| `.sym` | Symbols/source | Parsed as symbol source |
| `.chr`, `.ch8` | Charset asset | Editable in Char Editor |
| `.spr` | Sprite bank asset | Editable in Sprite Editor, 64 bytes per frame |
| `.bin`, `.raw`, `.dat` | Binary | Usable with `.incbin` / `.import binary` |
| `.sid`, `.prg`, `.seq`, `.scr`, `.koa` | Binary | Stored as project binary records |
| `.web64proj` | Project | Full Web64 IDE project |
| `.vsf` | Snapshot | Runtime/emulator state |

Other source-like extensions also accepted: `.def`, `.cfg`, `.map`.

## Adding files (Files panel)

- New: create a virtual file in the project tree.
- Add files: import one or more browser-selected files.
- Add folder: import a folder tree (when the browser supports directory selection).
- Remove: remove the selected virtual file.
- Clear: remove all virtual import files.

Added `.chr`/`.spr` files are converted into editable graphics assets with generated
include records. Text-like files become source records; other files become binary records.

## Creating files with New

The extension determines the initial type: `.chr`/`.ch8` → charset asset; `.spr` → sprite
bank asset; `.sym` → symbols/source file; text-like → editable source; anything else →
empty binary record.

## Path resolution order

When resolving an import path, the IDE tries:

1. The path relative to the current source file.
2. The direct normalized path.
3. The same path without leading parent segments.
4. A matching suffix in the virtual tree.
5. A matching base name, when unambiguous.

If a file cannot be found, the compiler reports an import diagnostic. If a base name is
ambiguous, use a more specific virtual path.
