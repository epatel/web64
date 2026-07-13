/* ---------------------------------------------------------------
   selftest.c — known-exact-value checks for the fixmath port.

   Each check computes into the zero-page registers and clears
   _selftest_ok on mismatch (asm compares; v0.1 C has none).
   Expected raw values are exact, so any pass is a real pass:
     umul16_c: $0280 * $0400        = $000a0000
     fmul:      2.5  *  4.0 = 10.0  ($0280 * $0400 -> $0a00)
     fdiv:    -10.0  /  4.0 = -2.5  ($f600 / $0400 -> $fd80)
     fsqrt:  sqrt(2.25)     =  1.5  ($0240         -> $0180)
   All calls go to the C fixmath.c routines (Phase 1); any port
   regression turns the border red immediately.
   --------------------------------------------------------------- */

#include <stdint.h>
#include "fixmath.h"

uint8_t selftest_ok = 0;

void fx_selftest(void) {
    selftest_ok = 1;

    /* umul16 (self-modifying, C module) */
    asm("
        lda #$80
        sta MULA
        lda #$02
        sta MULA+1
        lda #$00
        sta MULB
        lda #$04
        sta MULB+1
    ");
    umul16();
    asm("
        lda PROD
        bne st_f1
        lda PROD+1
        bne st_f1
        lda PROD+2
        cmp #$0a
        bne st_f1
        lda PROD+3
        beq st_p1
    st_f1:
        lda #$00
        sta _selftest_ok
    st_p1:
    ");

    /* fmul: 2.5 * 4.0 = 10.0 */
    asm("
        lda #$80
        sta FXA
        lda #$02
        sta FXA+1
        lda #$00
        sta FXB
        lda #$04
        sta FXB+1
    ");
    fmul();
    asm("
        lda FXR
        bne st_f2
        lda FXR+1
        cmp #$0a
        beq st_p2
    st_f2:
        lda #$00
        sta _selftest_ok
    st_p2:
    ");

    /* fdiv: -10.0 / 4.0 = -2.5 */
    asm("
        lda #$00
        sta FXA
        lda #$f6
        sta FXA+1
        lda #$00
        sta FXB
        lda #$04
        sta FXB+1
    ");
    fdiv();
    asm("
        lda FXR
        cmp #$80
        bne st_f3
        lda FXR+1
        cmp #$fd
        beq st_p3
    st_f3:
        lda #$00
        sta _selftest_ok
    st_p3:
    ");

    /* fsqrt: sqrt(2.25) = 1.5 */
    asm("
        lda #$40
        sta FXA
        lda #$02
        sta FXA+1
    ");
    fsqrt();
    asm("
        lda FXR
        cmp #$80
        bne st_f4
        lda FXR+1
        cmp #$01
        beq st_p4
    st_f4:
        lda #$00
        sta _selftest_ok
    st_p4:
    ");
}
