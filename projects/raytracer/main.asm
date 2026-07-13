; ---------------------------------------------------------------
; RAYTRACER — main  (project 2 of 2, consumes the fixmath lib)
;
; Shiny white (mirror + diffuse) sphere over a checkered floor,
; directional light with cast shadows, sky darkening toward the
; horizon. Dithered to 320x200 hires mono (DITHER_MODE below:
; Bayer / seeded white noise / blue noise — all animation-stable).
;
; Files:
;   main.asm          entry, render loop, dithering (this file)
;   scene.asm         scene constants (sphere, light, shades)
;   trace.asm         trace_pixel — the per-pixel ray math
;   lib/fixmath.asm   signed 8.8 fixed-point math
;   lib/gfx.asm       hires bitmap, plot, line, circle
;
; Full frame is minutes of C64 time — use warp. Optimization
; targets: table-based umul16, per-scanline constant folding.
;
; lib/ is shared from projects/fixmath/lib (source of truth).
; Code at $4000 (above the bitmap at $2000-$3f3f). Entry: start
; ---------------------------------------------------------------
* = $4000

PX      = $03           ; 2 bytes  current pixel x (0-319)
PY      = $05           ; 1 byte   current pixel y (0-199)
SHADE   = $06           ; 1 byte   pixel brightness 0-15

; Dithering — all modes are pure functions of (x, y), so every
; frame gets the identical pattern and animation won't flicker:
;   0 = ordered 4x4 Bayer (regular crosshatch)
;   1 = seeded white noise (Pearson hash of position, grainy)
;   2 = 16x16 blue noise (void-and-cluster, smooth AND random)
DITHER_MODE = 2
NOISE_SEED  = $5a       ; mode 1 only; any nonzero byte

    start:
        sei
        jsr fx_init     ; build multiply tables (required by fixmath)
        jsr gfx_init
        jsr noise_init
        jsr render
    forever:
        jmp forever

; --- render: for every pixel, trace then dither ------------------
    render:
        lda #$00
        sta PY
    rn_yloop:
        lda #$00
        sta PX
        sta PX+1
    rn_xloop:
        jsr trace_pixel ; -> SHADE (0-15)
        ; threshold: plot if SHADE > threshold(x, y)
        lda #DITHER_MODE
        beq rn_bayer
        cmp #$02
        beq rn_blue
        ; seeded noise: Pearson hash T[T[x] ^ y] & 15 — consecutive
        ; LFSR entries correlate, so hash instead of walking ntab
        lda PX
        clc
        adc PX+1
        tax
        lda ntab,x
        eor PY
        tax
        lda ntab,x
        and #$0f
        jmp rn_cmp
    rn_blue:
        ; blue noise: bnoise[(y&15)*16 + (x&15)]
        lda PY
        and #$0f
        asl
        asl
        asl
        asl
        sta BAYIX
        lda PX
        and #$0f
        clc
        adc BAYIX
        tax
        lda bnoise,x
        jmp rn_cmp
    rn_bayer:
        ; ordered Bayer 4x4: bayer[(py&3)*4 + (px&3)]
        lda PY
        and #$03
        asl
        asl
        sta BAYIX
        lda PX
        and #$03
        clc
        adc BAYIX
        tax
        lda bayer4,x
    rn_cmp:
        sta DTH
        lda SHADE
        cmp DTH
        bcc rn_dark
        beq rn_dark
        lda PX
        sta GPX
        lda PX+1
        sta GPX+1
        lda PY
        sta GPY
        jsr gfx_plot
    rn_dark:
        inc PX
        bne rn_xchk
        inc PX+1
    rn_xchk:
        lda PX+1
        cmp #>320
        bcc rn_xloop
        lda PX
        cmp #<320
        bcc rn_xloop
        inc PY
        lda PY
        cmp #200
        bcs rn_done
        jmp rn_yloop
    rn_done:
        rts

; --- noise_init: fill ntab deterministically from NOISE_SEED -----
; Galois LFSR (taps $b8, maximal 255-cycle) — same seed, same
; table, same dither pattern on every run.
    noise_init:
        lda #NOISE_SEED
        ldx #$00
    ni_loop:
        lsr
        bcc ni_skip
        eor #$b8
    ni_skip:
        sta ntab,x
        inx
        bne ni_loop
        rts

    bayer4:
        .byte  0,  8,  2, 10
        .byte 12,  4, 14,  6
        .byte  3, 11,  1,  9
        .byte 15,  7, 13,  5

    BAYIX:  .byte 0
    DTH:    .byte 0
    ntab:
        .fill 256, 0

; 16x16 blue-noise threshold matrix, 16 levels (0-15), generated
; offline with the void-and-cluster algorithm (seed 42, sigma 1.5,
; toroidal). Tiles seamlessly; regenerate via project notes.
    bnoise:
        .byte  8,  0, 15,  4,  2, 11,  0,  5, 11,  9, 10,  3, 12, 13,  4,  6
        .byte 12,  3,  7, 10,  8, 13,  4, 15,  3, 13,  1,  5,  7,  2, 10, 15
        .byte  2,  9, 14,  1,  6,  2,  9,  8,  1,  6, 12, 15, 11,  8,  4, 11
        .byte  7,  4, 12,  3, 15, 12,  7, 11, 14,  4,  8,  0,  3, 14,  1,  5
        .byte 15,  0, 10,  7,  9,  3,  0,  5,  2, 11,  9,  5,  7, 13,  9, 12
        .byte  8,  6, 14,  1,  5, 14, 10, 13,  6, 15,  1, 14, 11,  6,  4,  1
        .byte 10,  3,  2, 12,  6, 11,  4,  8,  0,  7, 12,  3,  0,  9,  2, 14
        .byte  5, 13,  9, 15,  0,  8,  2, 15, 10,  2,  5,  8, 10, 15, 12,  7
        .byte 11,  1,  7,  4, 10, 13,  6, 12,  4, 13, 11, 14,  1,  6,  3,  0
        .byte 15,  6, 13,  2,  5,  3,  9,  1,  6,  8,  0,  4,  5, 10, 13,  9
        .byte  2,  3,  9, 14, 11,  8, 14,  4, 14,  2,  9, 15, 12,  2,  7,  5
        .byte 10, 13,  8,  0,  7,  1, 11,  7, 11, 13,  3,  6,  8,  0, 14, 12
        .byte  0,  5,  3, 12,  4, 13,  2,  5,  0,  5, 10,  1, 13, 10,  4,  6
        .byte 14, 11,  7, 15, 10,  6, 15,  9, 13,  8, 15,  4, 12,  5,  2,  8
        .byte  3,  5,  1,  9,  1,  3, 10,  1,  4, 11,  0,  7,  3, 11, 15, 10
        .byte 13,  9, 12,  6, 14,  8, 12,  7, 14,  2,  6, 14,  9,  0,  7,  1

; --- project files ------------------------------------------------
        .include "scene.asm"
        .include "trace.asm"
        .include "lib/fixmath.asm"
        .include "lib/gfx.asm"
