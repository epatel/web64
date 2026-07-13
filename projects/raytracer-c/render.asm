; ---------------------------------------------------------------
; RAYTRACER-C — assembly support (init wrappers, noise, tables)
;
; The render loop itself lives in main.c as a C function with
; inline-asm control-flow scaffolding; this module keeps only what
; the C subset genuinely cannot hold:
;   - the zero-page equates for the trace kernel interface
;   - asm_* init wrappers C calls via `jsr asm_*`
;   - noise_init (LFSR needs shifts/carry)
;   - the dither threshold tables (no C arrays)
; Inline asm in main.c references PX/PY/SHADE, BAYIX, DTH, bayer4,
; ntab, bnoise, trace_pixel, GPX/GPY, gfx_plot directly — one
; shared assembler symbol namespace.
;
; No org here: code flows after the generated C module from the
; project origin ($4000, above the bitmap at $2000-$3f3f).
;
; scene.asm / trace.asm / lib are shared with projects/raytracer
; and projects/fixmath (symlinks; those projects are the source
; of truth) — the per-pixel kernel is byte-identical.
; ---------------------------------------------------------------

PX      = $03           ; 2 bytes  current pixel x (0-319)
PY      = $05           ; 1 byte   current pixel y (0-199)
SHADE   = $06           ; 1 byte   pixel brightness 0-15

; --- fixmath zero page + tables (moved from lib/fixmath.asm when
; the lib was replaced by fixmath.c; the C asm blocks, selftest.c,
; and trace.asm all reference these) ------------------------------
SQR1LO  = $c000         ; quarter-square tables, 512 bytes each
SQR1HI  = $c200
SQR2LO  = $c400
SQR2HI  = $c600

MULA    = $57           ; 2 bytes  multiplier
MULB    = $59           ; 2 bytes  multiplicand (destroyed)
PROD    = $5b           ; 4 bytes  product
DVD     = $5f           ; 3 bytes  dividend / quotient
DVS     = $62           ; 2 bytes  divisor
REM     = $64           ; 2 bytes  remainder
SQN     = $66           ; 3 bytes  sqrt input (destroyed)
SQROOT  = $69           ; 2 bytes  sqrt result
SQREM   = $6b           ; 2 bytes  sqrt scratch
SGN     = $6d           ; 1 byte   sign scratch
FXR     = $6e           ; 2 bytes  fixed-point result

FXA     = MULA          ; fixed-point operand aliases
FXB     = MULB

UT1     = DVD           ; umul16 temps (overlay divide registers)
UT2     = DVS
UT3     = DVD+2

; --- transition bridges: asm callers (trace.asm) -> C fixmath ----
; Removed in Phase 3 when trace.asm becomes trace.c.
fmul:
        jmp _fmul
fdiv:
        jmp _fdiv
fsqrt:
        jmp _fsqrt

; --- C-callable wrappers -----------------------------------------
asm_gfx_init:
        jmp gfx_init

; --- asm_noise_init: fill ntab deterministically from _noise_seed
; Galois LFSR (taps $b8, maximal 255-cycle) — same seed, same
; table, same dither pattern on every run.
asm_noise_init:
        lda _noise_seed
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
; toroidal). Tiles seamlessly; regenerate via raytracer project
; tools/bluenoise.py.
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
