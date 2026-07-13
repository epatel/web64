# sprite-editor

The browser-local C64 hardware sprite editor: data model, mono/multicolor modes, overlay
pairs, animation preview, and workflow.

## Sprite data model

A C64 hardware sprite is 24x21 pixels, 63 bytes of pixel data, plus 1 padding byte in
Web64 IDE sprite banks = 64 bytes per exported frame. The padding byte exists because
VIC-II sprite pointers address 64-byte sprite slots.

## Creating sprite assets

Use New Sprite Bank to create a single ordinary sprite bank; default path is like
`assets/sprites/sprites.spr`.

Use New Pair to create one logical sprite made from two ordinary sprite banks:

```
assets/sprites/player_mc.spr          ; multicolor base layer
assets/sprites/player_ol.spr          ; monochrome overlay layer
assets/sprites/player.spritepair.json ; editor metadata (name, layer paths,
                                      ;   overlay preview color, opacity)
assets/sprites/player.inc
```

Overlay pairing is an editor abstraction over ordinary sprite assets, not a new binary
format — the `.spr` bytes remain standard 64-byte sprite records, and existing `.spr`
projects work unchanged.

## Layout

- Logical/raw asset selector: groups paired `_mc`/`_ol` files by default; Raw mode
  inspects the underlying assets.
- Frame list with selectable previews; frame actions: add, duplicate, copy, paste,
  delete, move left/right.
- Pixel editor (zoomed current frame) and toolbar: pencil, erase, fill, flip, shift.
- Pair mode control: Multicolor, Overlay, Composite, Split (for paired sprites).
- Preview of the current frame or composited pair.
- Animation controls: play/pause, pingpong, onion skin, FPS.
- Properties: frame index, byte offset, frame size, label, include path.
- Palette and color slots; generated include preview.

## Pair modes

- Multicolor: edits only the `_mc` base asset.
- Overlay: edits only the `_ol` asset, with the multicolor base shown as context.
- Composite: read-only preview of the two hardware sprites at the same X/Y, overlay
  pixels drawn above base pixels.
- Split: side-by-side cleanup, same logical pair selection and synchronized frame index.

Frame add/duplicate/delete/move apply to BOTH layer banks when a logical pair is
selected. If pair frame counts diverge, the editor shows repair diagnostics rather than
silently treating the layers as synchronized.

## Mono mode

Pixel values 0 (transparent/background) and 1 (sprite color). Rows export as 3 bytes per
row, high bit leftmost.

## Multicolor mode

Logical size 12x21; each logical pixel is a 2-bit horizontal pair. Color slots:
0 = transparent/background, 1 = sprite multicolor 1, 2 = sprite-specific color,
3 = sprite multicolor 2. The third slot is intentionally the sprite-specific color because
that is the color normally set per hardware sprite.

## Pair include output

Pair includes generate deterministic `_mc` and `_ol` symbol groups plus pair convenience
labels. For `Player`:

```asm
player_mc_sprites_size = ...
player_mc_sprite_count = ...
player_mc_frame_0 = player_mc_sprites + 0

player_ol_sprites_size = ...
player_ol_sprite_count = ...
player_ol_frame_0 = player_ol_sprites + 0

player_sprite_count = player_mc_sprite_count
player_mc = player_mc_sprites
player_ol = player_ol_sprites
```

Single-sprite include output is unchanged.

## Animation preview

Runs from JavaScript editor memory, not emulator memory, keeping playback responsive
while editing. Controls: play/pause, pingpong, FPS slider, onion skin (neighboring frames
at low opacity for animation continuity). For paired sprites, animation advances both
layers with the same frame index. Pair overlay color and opacity are editor preview
settings only — they don't change exported `.spr` bytes.

## Troubleshooting pairs

- Missing layer: create or restore the referenced `_mc.spr` or `_ol.spr` file, or switch
  to Raw mode to inspect the underlying assets.
- Frame count mismatch: restore both layers to a matching count, then use paired frame
  operations from the logical entry.
- Manual include files are not the source of truth; regenerate pair includes from the
  sprite assets and descriptor when needed.

## Workflow: add a sprite bank to a program

1. Create e.g. `assets/sprites/player.spr`, draw frames, set multicolor mode and color
   slots if needed.
2. In source:

```asm
* = $3000
.import binary player_sprites, "assets/sprites/player.spr"
.include "assets/sprites/player.inc"
```

3. Use `player_frame_0`, `player_sprite_count`, and `player_sprite_color` in code.
4. Start PRG; edit the sprite and observe live patching if the program uses the imported
   bytes directly.
