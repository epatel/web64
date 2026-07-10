# char-editor

The browser-local C64 character set editor: data model, modes, tools, and workflow.

## Character data model

A C64 character is 8 bytes: 8 rows, 1 byte per row. Mono mode is 1 bit per pixel;
multicolor mode uses 2-bit pairs per row, giving 4 logical columns rendered as
double-width pixels. A full character set is commonly `256 chars * 8 bytes = 2048 bytes`.

## Creating a charset

Use New Charset in the Char Editor. Default path is like `assets/chars/charset.chr`. The
editor adds the binary asset and its generated include file to the virtual filesystem.

## Layout

- Charset grid: 16x16 character preview grid.
- Pixel editor: zoomed editor for the selected character.
- Toolbar: pencil, erase, fill, copy, paste, duplicate, flip, shift.
- Properties: index, hex index, byte offset, label, include path.
- 3x3 tile preview: the selected character repeated, for designing seamless tiles.
- Palette and color slots.
- Generated include preview.

## Mono mode

Pixel values 0 (background color) and 1 (foreground color). Exported bytes are standard
C64 character bytes.

## Multicolor mode

Pixel values 0–3: 0 = background/global color, 1 = multicolor 1, 2 = multicolor 2,
3 = character-specific foreground color. The editor paints 2-bit horizontal pairs;
preview pixels are shown double-wide to match C64 multicolor geometry.

## Tools

Pencil (paint selected value), erase (paint zero), fill (fill current character),
copy/paste, duplicate (copy into next slot), flip X/Y, shift left/right/up/down.
Mouse drag continues painting; right mouse or erase paints zero; touch/pointer drawing is
supported where the browser provides pointer events.

## Workflow: add a character set to a program

1. Create e.g. `assets/chars/tiles.chr`, draw characters, use the 3x3 preview for tile
   continuity.
2. In source:

```asm
* = $3800
.import binary tiles_chars, "assets/chars/tiles.chr"
.include "assets/chars/tiles.inc"
```

3. Copy the charset to VIC-visible RAM or point VIC bank/screen setup at the imported
   location as appropriate.
