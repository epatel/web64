/* ---------------------------------------------------------------
   fixmath.c — C port of lib/fixmath.asm (Phase 0: umul16 spike).

   The routine body is the lib's umul16 verbatim inside one asm()
   block: quarter-square multiply with SELF-MODIFYING operand
   patches (the `sta cua1+1` stores rewrite the table addresses of
   the lda/sbc below). Labels are renamed ua*->cua*, ub*->cub* so
   they don't collide with lib/fixmath.asm's copy while both are in
   the build. Zero-page equates (MULA, MULB, PROD, UT1-UT3) come
   from lib/fixmath.asm — one shared assembler namespace.

   The C epilogue supplies the rts.
   --------------------------------------------------------------- */

#include "fixmath.h"

void umul16_c(void) {
    asm("    lda MULA\n    sta cua1+1\n    sta cua2+1\n    sta cua5+1\n    sta cua6+1\n    eor #$ff\n    sta cua3+1\n    sta cua4+1\n    sta cua7+1\n    sta cua8+1\n    lda MULA+1\n    sta cub1+1\n    sta cub2+1\n    sta cub5+1\n    sta cub6+1\n    eor #$ff\n    sta cub3+1\n    sta cub4+1\n    sta cub7+1\n    sta cub8+1\n    ldy MULB\n    sec\ncua1:    lda SQR1LO,y\ncua3:    sbc SQR2LO,y\n    sta PROD\ncua2:    lda SQR1HI,y\ncua4:    sbc SQR2HI,y\n    sta PROD+1\n    sec\ncub1:    lda SQR1LO,y\ncub3:    sbc SQR2LO,y\n    sta UT2\ncub2:    lda SQR1HI,y\ncub4:    sbc SQR2HI,y\n    sta UT2+1\n    ldy MULB+1\n    sec\ncua5:    lda SQR1LO,y\ncua7:    sbc SQR2LO,y\n    sta UT1\ncua6:    lda SQR1HI,y\ncua8:    sbc SQR2HI,y\n    sta UT1+1\n    sec\ncub5:    lda SQR1LO,y\ncub7:    sbc SQR2LO,y\n    sta PROD+2\ncub6:    lda SQR1HI,y\ncub8:    sbc SQR2HI,y\n    sta PROD+3\n    clc\n    lda UT1\n    adc UT2\n    sta UT1\n    lda UT1+1\n    adc UT2+1\n    sta UT1+1\n    lda #$00\n    adc #$00\n    sta UT3\n    clc\n    lda PROD+1\n    adc UT1\n    sta PROD+1\n    lda PROD+2\n    adc UT1+1\n    sta PROD+2\n    lda PROD+3\n    adc UT3\n    sta PROD+3");
}
