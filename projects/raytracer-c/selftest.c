/* ---------------------------------------------------------------
   selftest.c — known-exact-value checks for the fixmath port.

   Each check computes into the zero-page registers and clears
   _selftest_ok on mismatch (asm compares; v0.1 C has none).
   Expected raw values are exact, so any pass is a real pass:
     umul16_c: $0280 * $0400        = $000a0000
     fmul:      2.5  *  4.0 = 10.0  ($0280 * $0400 -> $0a00)
     fdiv:    -10.0  /  4.0 = -2.5  ($f600 / $0400 -> $fd80)
     fsqrt:  sqrt(2.25)     =  1.5  ($0240         -> $0180)
   fmul/fdiv/fsqrt currently still call the asm lib routines; as
   Phase 1 converts each one, this file keeps passing (or catches
   the regression immediately).
   --------------------------------------------------------------- */

#include <stdint.h>
#include "fixmath.h"

uint8_t selftest_ok = 0;

void fx_selftest(void) {
    selftest_ok = 1;

    /* umul16_c (C module, self-modifying spike) */
    asm("    lda #$80\n    sta MULA\n    lda #$02\n    sta MULA+1\n    lda #$00\n    sta MULB\n    lda #$04\n    sta MULB+1");
    umul16_c();
    asm("    lda PROD\n    bne st_f1\n    lda PROD+1\n    bne st_f1\n    lda PROD+2\n    cmp #$0a\n    bne st_f1\n    lda PROD+3\n    beq st_p1\nst_f1:\n    lda #$00\n    sta _selftest_ok\nst_p1:");

    /* fmul: 2.5 * 4.0 = 10.0 */
    asm("    lda #$80\n    sta FXA\n    lda #$02\n    sta FXA+1\n    lda #$00\n    sta FXB\n    lda #$04\n    sta FXB+1\n    jsr fmul\n    lda FXR\n    bne st_f2\n    lda FXR+1\n    cmp #$0a\n    beq st_p2\nst_f2:\n    lda #$00\n    sta _selftest_ok\nst_p2:");

    /* fdiv: -10.0 / 4.0 = -2.5 */
    asm("    lda #$00\n    sta FXA\n    lda #$f6\n    sta FXA+1\n    lda #$00\n    sta FXB\n    lda #$04\n    sta FXB+1\n    jsr fdiv\n    lda FXR\n    cmp #$80\n    bne st_f3\n    lda FXR+1\n    cmp #$fd\n    beq st_p3\nst_f3:\n    lda #$00\n    sta _selftest_ok\nst_p3:");

    /* fsqrt: sqrt(2.25) = 1.5 */
    asm("    lda #$40\n    sta FXA\n    lda #$02\n    sta FXA+1\n    jsr fsqrt\n    lda FXR\n    cmp #$80\n    bne st_f4\n    lda FXR+1\n    cmp #$01\n    beq st_p4\nst_f4:\n    lda #$00\n    sta _selftest_ok\nst_p4:");
}
