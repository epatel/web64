# generated-includes

Include files auto-generated from charset and sprite assets: the labels they expose and
how to consume them.

Graphics assets generate include files, visible in the project tree as generated virtual
files and previewed in the asset editor. They are meant to be consumed by assembly —
edit the asset, not the generated include. The label base comes from the asset file name
(`assets/chars/logo.chr` → `logo`; `assets/sprites/player.spr` → `player`).

## Charset include labels

```asm
logo_chars_size = $0800
logo_color_0 = 0
logo_background_color = 0
logo_color_1 = 5
logo_multicolor_1 = 5
logo_color_2 = 2
logo_multicolor_2 = 2
logo_color_3 = 1
logo_foreground_color = 1
logo_char_count = $0100
logo_chars_end = logo_chars + logo_chars_size
logo_char_0 = 0
logo_char_1 = 1
```

Charset color constants: `_color_0`/`_background_color` = background/global color,
`_color_1`/`_multicolor_1`, `_color_2`/`_multicolor_2`,
`_color_3`/`_foreground_color` = foreground/character color.

## Sprite include labels

```asm
player_sprites_size = $80
player_color_0 = 0
player_background_color = 0
player_color_1 = 6
player_multicolor_1 = 6
player_color_2 = $0c
player_sprite_color = $0c
player_color_3 = $0e
player_multicolor_2 = $0e
player_sprite_count = 2
player_sprites_end = player_sprites + player_sprites_size
player_frame_0 = player_sprites + 0
player_frame_1 = player_sprites + $40
```

Sprite color constants: `_color_0`/`_background_color` = transparent/background,
`_color_1`/`_multicolor_1` = sprite multicolor 1,
`_color_2`/`_sprite_color` = sprite-specific color (use this for the per-sprite VIC-II
color value), `_color_3`/`_multicolor_2` = sprite multicolor 2.

## Usage

```asm
* = $3000
.import binary player_sprites, "assets/sprites/player.spr"
.include "assets/sprites/player.inc"
```

## Minimal complete example

```asm
* = $c000

BORDER = $d020
BG = $d021

.include "assets/sprites/player.inc"

start:
    sei
    lda #0
    sta BORDER
    sta BG

    lda #<player_sprites
    sta $07f8
    lda #player_sprite_color
    sta $d027

main:
    inc BORDER
    jmp main

* = $3000
.import binary player_sprites, "assets/sprites/player.spr"
```

Save the sprite asset at `assets/sprites/player.spr`, then save the project. The
generated include exposes the color and frame constants; `.import binary` inserts the
sprite bytes into the PRG.
