/* ---------------------------------------------------------------
   RAYTRACER-C — Web64 C v0.1 edition of the raytracer.

   C owns the entry point, the startup sequence, the tuning globals,
   AND the render loop below. Web64 C v0.1 has no loop statements,
   arrays, arithmetic, or comparisons, so control flow is scaffolded
   with inline asm labels/branches around lowered C statements
   (emission order is linear, so a backward `bne` over C code works
   — verified). The 8.8 fixed-point kernel (trace_pixel, fixmath,
   gfx) stays in assembly modules; short asm blocks here marshal
   C globals into its zero-page interface and do the table lookups
   the C subset cannot express.

   NOT possible instead: a macro utility header. Function-like
   #define is not expanded at all, and object-like #define expands
   only in expression position (constants) — tested. Only include
   guards and constant defines survive preprocessing.

   C globals are exported with underscore labels (_px, _py,
   _dither_mode); asm_* calls lower to `jsr asm_*` (render.asm).
   --------------------------------------------------------------- */

#include <stdint.h>
#include "fixmath.h"
#include "gfx.h"
#include "trace.h"
#include "selftest.h"

/* Dither mode, read per pixel at runtime. 0 = ordered 4x4 Bayer,
   1 = seeded white noise, 2 = 16x16 blue noise (default). All are
   pure functions of (x, y): animation-stable, identical image on
   every run. */
uint8_t dither_mode = 2;

/* White-noise LFSR seed, mode 1 only; any nonzero byte. */
uint8_t noise_seed = 0x5a;

/* Render loop counters. uint16_t px lowers to little-endian data
   bytes _px / _px+1 (the initializer emits a proper .WORD), but all
   RUNTIME operations on word globals are low-byte-only in v0.1 —
   assignment, ++, compares. Every 16-bit manipulation of px below
   is therefore inline asm; C only declares the storage. */
uint16_t px = 0;
uint8_t py = 0;

/* plot decision flag, set per pixel by an asm compare (v0.1 C has
   no >): 1 when SHADE > threshold */
uint8_t do_plot = 0;

/* --- render: for every pixel, trace then dither ----------------
   Same algorithm as projects/raytracer main.asm `render`, with the
   loop skeleton and mode dispatch lifted into C. trace_pixel takes
   PX at $03/$04, PY at $05 and returns SHADE at $06 (zero page). */
void render(void) {
    py = 0;
    asm("rn_yloop:");
    /* px = 0 in asm: like px++, a C assignment to a uint16_t only
       writes the low byte (verified: rows 1+ rendered from x=256
       because _px+1 kept the previous row's exit value) */
    asm("    lda #0\n    sta _px\n    sta _px+1");
    asm("rn_xloop:");

    /* marshal px/py into the kernel zero page, trace the pixel */
    asm("    lda _px\n    sta PX\n    lda _px+1\n    sta PX+1\n    lda _py\n    sta PY");
    trace_pixel();

    /* threshold(x, y) -> DTH: compute Bayer unconditionally, then
       let the mode bits override — flat ifs, no else needed
       (mode 0 keeps Bayer, 1 overrides with noise, 2 with blue) */
    asm("    lda _py\n    and #$03\n    asl\n    asl\n    sta BAYIX\n    lda _px\n    and #$03\n    clc\n    adc BAYIX\n    tax\n    lda bayer4,x\n    sta DTH");
    if (dither_mode & 1) {
        /* seeded noise: Pearson hash T[T[x] ^ y] & 15 */
        asm("    lda _px\n    clc\n    adc _px+1\n    tax\n    lda ntab,x\n    eor _py\n    tax\n    lda ntab,x\n    and #$0f\n    sta DTH");
    }
    if (dither_mode & 2) {
        /* blue noise: bnoise[(y&15)*16 + (x&15)] */
        asm("    lda _py\n    and #$0f\n    asl\n    asl\n    asl\n    asl\n    sta BAYIX\n    lda _px\n    and #$0f\n    clc\n    adc BAYIX\n    tax\n    lda bnoise,x\n    sta DTH");
    }

    /* plot if SHADE > DTH — the compare sets _do_plot, C branches */
    asm("    ldx #$00\n    lda SHADE\n    cmp DTH\n    bcc rn_dark\n    beq rn_dark\n    inx\nrn_dark:\n    stx _do_plot");
    if (do_plot & 1) {
        asm("    lda _px\n    sta GPX\n    lda _px+1\n    sta GPX+1\n    lda _py\n    sta GPY");
        gfx_plot();
    }

    /* px++ in asm: the v0.1 lowering of a uint16_t increment does
       not carry into the high byte (verified: the x loop wrapped at
       256 and the render hung on scanline 0) */
    asm("    inc _px\n    bne rn_xinc\n    inc _px+1\nrn_xinc:");
    /* while (px < 320) — via jmp trampoline: the C-lowered loop
       body is too long for a direct backward branch (-128..127) */
    asm("    lda _px+1\n    cmp #>320\n    bcc rn_xcont\n    lda _px\n    cmp #<320\n    bcc rn_xcont\n    jmp rn_xdone\nrn_xcont:\n    jmp rn_xloop\nrn_xdone:");
    py++;
    asm("    lda _py\n    cmp #200\n    bcs rn_done\n    jmp rn_yloop\nrn_done:");
}

void main(void) {
    asm("    sei");
    fx_init();         /* quarter-square tables at $c000 — required
                          (C fixmath.c since Phase 1)               */

    /* Fixmath smoke test (seconds, vs minutes for a render):
       uncomment to check, border green = pass, red = fail.
    fx_selftest();
    if (selftest_ok & 1) { asm("    lda #$05\n    sta $d020"); }
    if (!(selftest_ok & 1)) { asm("    lda #$02\n    sta $d020"); }
    asm("st_halt:\n    jmp st_halt");
    */

    gfx_init();        /* hires bitmap at $2000, cleared (C gfx.c)  */
    asm_noise_init();  /* LFSR noise table from _noise_seed         */
    render();          /* C render loop above                       */
    asm("halt:\n    jmp halt");
}
