# sprite-editor

The browser-local C64 hardware sprite editor: data model, modes, animation preview, and
workflow.

## Sprite data model

A C64 hardware sprite is 24x21 pixels, 63 bytes of pixel data, plus 1 padding byte in
Web64 IDE sprite banks = 64 bytes per exported frame. The padding byte exists because
VIC-II sprite pointers address 64-byte sprite slots.

## Creating a sprite bank

Use New Sprite Bank in the Sprite Editor. Default path is like `assets/sprites/sprites.spr`.
The editor adds the binary asset and generated include file to the virtual filesystem.

## Layout

- Frame list with selectable previews; frame actions: add, duplicate, copy, paste,
  delete, move left/right.
- Pixel editor (zoomed current frame) and toolbar: pencil, erase, fill, flip, shift.
- Preview of the current frame.
- Animation controls: play/pause, pingpong, onion skin, FPS.
- Properties: frame index, byte offset, frame size, label, include path.
- Palette and color slots; generated include preview.

## Mono mode

Pixel values 0 (transparent/background) and 1 (sprite color). Rows export as 3 bytes per
row, high bit leftmost.

## Multicolor mode

Logical size 12x21; each logical pixel is a 2-bit horizontal pair. Color slots:
0 = transparent/background, 1 = sprite multicolor 1, 2 = sprite-specific color,
3 = sprite multicolor 2. The third slot is intentionally the sprite-specific color because
that is the color normally set per hardware sprite.

## Animation preview

Runs from JavaScript editor memory, not emulator memory, keeping playback responsive
while editing. Controls: play/pause, pingpong, FPS slider, onion skin (neighboring frames
at low opacity for animation continuity).

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
