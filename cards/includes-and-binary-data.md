# includes-and-binary-data

Including source files, importing symbols, and embedding binary data from the virtual
filesystem.

All paths refer to virtual project paths with forward slashes.

## Source include

```asm
.include "includes/common.asm"
.include "assets/sprites/player.inc"
```

Accepted forms: `.include "path"`, `!include "path"`, `include "path"`, `#include "path"`.

## Source import (explicit form)

```asm
.import source "Main-CommonDefines.asm"
.import asm "path"
.import include "path"
```

## Symbol import

Symbol files are text files exposing addresses or constants. `.sym` files are treated as
symbol files automatically.

```asm
.import symbols "build/main.sym"
.import source "../../Out/6502/Main/Main-BaseCode.sym"
.include "assets/chars/logo.inc"
```

Supported symbol line formats:

```asm
label = $c000
label equ $c000
$c000 label
label $c000
```

Generated include files from graphics assets use simple assignments, so they can be
consumed directly by the assembler.

## Binary import with a label

`.import binary` inserts a binary file at the current assembly address and binds a label
to the start:

```asm
* = $3000
.import binary player_sprites, "assets/sprites/player.spr"

lda #<player_sprites
ldx #>player_sprites
```

## Incbin

`.incbin` inserts raw bytes from a virtual binary file, optionally with a symbol:

```asm
.incbin "assets/chars/logo.chr"
.incbin logo_chars "assets/chars/logo.chr"
.incbin player_sprites, "assets/sprites/player.spr"
```

Accepted binary directive families: `.incbin`, `!incbin`, `incbin`, `.bin`, `!bin`, `bin`,
`.binary`, `!binary`, `binary`, `.import binary`, `.import bin`.

## Workflow: split source files

1. Add virtual files such as `includes/constants.asm`, `includes/macros.asm`,
   `src/raster.asm`.
2. In the main source:

```asm
.include "includes/constants.asm"
.include "includes/macros.asm"
.include "src/raster.asm"
```

3. Keep paths project-relative and save the project when the virtual file tree changes.

## Workflow: import symbols from another build

1. Add a `.sym` file to the virtual tree.
2. Include it with `.import source "Main-BaseCode.sym"` or `.include "Main-BaseCode.sym"`.
3. Use the imported labels in expressions.

## Recommended pattern: binary assets under a symbol

For graphics, music, maps, and lookup tables:

```asm
* = $3000
.import binary player_sprites, "assets/sprites/player.spr"
.include "assets/sprites/player.inc"

* = $3800
.import binary logo_chars, "assets/chars/logo.chr"
.include "assets/chars/logo.inc"
```

This gives both the binary data and labels for sizes, counts, frame offsets, character
indices, and editor color references.
