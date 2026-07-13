/* ---------------------------------------------------------------
   fixmath.c — C port of lib/fixmath.asm (signed 8.8 fixed point).

   Same algorithms, same zero-page interface (see fixmath.h and the
   equates in render.asm), byte-for-byte the same math. C owns the
   function decomposition and the call graph (fmul -> umul16,
   fdiv -> udiv24, fsqrt -> isqrt24 are C calls); the arithmetic
   bodies are inline asm — v0.1 has no expressions or loops.
   Internal labels are prefixed c* (cua1, cfm_apos, ...) so they
   never collide with the asm lib during transition.

   API (all zero page, values are raw 8.8 two's complement):
     fx_init : build quarter-square tables at $c000 — CALL ONCE
     umul16  : PROD(32u) = MULA(16u) * MULB(16u)  [preserves inputs]
     fmul    : FXR = FXA * FXB   [FXA/FXB destroyed; |a*b| < 128]
     fdiv    : FXR = FXA / FXB   [saturates to +/-$7fff]
     isqrt24 : SQROOT = floor(sqrt(SQN(24u)))     [clobbers DVD]
     fsqrt   : FXR = sqrt(FXA), FXA >= 0

   asm callers (trace.asm) reach fmul/fdiv/fsqrt through bridge
   labels in render.asm (fmul: jmp _fmul, ...) until Phase 3.
   --------------------------------------------------------------- */

#include "fixmath.h"

/* --- fx_init: build the quarter-square tables (call once) -------
   f(n) = floor(n*n/4), built incrementally: f(n+1) = f(n) +
   floor((n+1)/2). SQR2[i] = f(|i-255|) is copied from SQR1.
   Uses SQN/SQROOT as pointers and REM/DVS as accumulator/delta. */
void fx_init(void) {
    asm("    lda #<SQR1LO\n    sta SQN\n    lda #>SQR1LO\n    sta SQN+1\n    lda #<SQR1HI\n    sta SQROOT\n    lda #>SQR1HI\n    sta SQROOT+1\n    lda #$00\n    sta REM\n    sta REM+1\n    sta DVS\n    tay\n    ldx #$02\ncfxi_loop:\n    lda REM\n    sta (SQN),y\n    lda REM+1\n    sta (SQROOT),y\n    tya\n    and #$01\n    beq cfxi_add\n    inc DVS\ncfxi_add:\n    clc\n    lda REM\n    adc DVS\n    sta REM\n    lda REM+1\n    adc #$00\n    sta REM+1\n    iny\n    bne cfxi_loop\n    inc SQN+1\n    inc SQROOT+1\n    dex\n    bne cfxi_loop\n    ldx #$00\n    ldy #$ff\ncfxi_c1:\n    lda SQR1LO,y\n    sta SQR2LO,x\n    lda SQR1HI,y\n    sta SQR2HI,x\n    dey\n    inx\n    bne cfxi_c1\ncfxi_c2:\n    lda SQR1LO+1,x\n    sta SQR2LO+$100,x\n    lda SQR1HI+1,x\n    sta SQR2HI+$100,x\n    inx\n    bne cfxi_c2");
}

/* --- umul16: PROD = MULA * MULB (unsigned 16x16 -> 32) ----------
   Quarter-square multiply, four 8x8 partial products. SELF-
   MODIFYING: the operand low bytes are patched into the table
   addresses (`sta cua1+1` rewrites the lda below). Verbatim from
   the asm lib. Requires fx_init. Preserves MULA/MULB. */
void umul16(void) {
    asm("    lda MULA\n    sta cua1+1\n    sta cua2+1\n    sta cua5+1\n    sta cua6+1\n    eor #$ff\n    sta cua3+1\n    sta cua4+1\n    sta cua7+1\n    sta cua8+1\n    lda MULA+1\n    sta cub1+1\n    sta cub2+1\n    sta cub5+1\n    sta cub6+1\n    eor #$ff\n    sta cub3+1\n    sta cub4+1\n    sta cub7+1\n    sta cub8+1\n    ldy MULB\n    sec\ncua1:    lda SQR1LO,y\ncua3:    sbc SQR2LO,y\n    sta PROD\ncua2:    lda SQR1HI,y\ncua4:    sbc SQR2HI,y\n    sta PROD+1\n    sec\ncub1:    lda SQR1LO,y\ncub3:    sbc SQR2LO,y\n    sta UT2\ncub2:    lda SQR1HI,y\ncub4:    sbc SQR2HI,y\n    sta UT2+1\n    ldy MULB+1\n    sec\ncua5:    lda SQR1LO,y\ncua7:    sbc SQR2LO,y\n    sta UT1\ncua6:    lda SQR1HI,y\ncua8:    sbc SQR2HI,y\n    sta UT1+1\n    sec\ncub5:    lda SQR1LO,y\ncub7:    sbc SQR2LO,y\n    sta PROD+2\ncub6:    lda SQR1HI,y\ncub8:    sbc SQR2HI,y\n    sta PROD+3\n    clc\n    lda UT1\n    adc UT2\n    sta UT1\n    lda UT1+1\n    adc UT2+1\n    sta UT1+1\n    lda #$00\n    adc #$00\n    sta UT3\n    clc\n    lda PROD+1\n    adc UT1\n    sta PROD+1\n    lda PROD+2\n    adc UT1+1\n    sta PROD+2\n    lda PROD+3\n    adc UT3\n    sta PROD+3");
}

/* --- fmul: FXR = FXA * FXB (signed 8.8) ------------------------- */
void fmul(void) {
    /* result sign, |FXA|, |FXB| */
    asm("    lda FXA+1\n    eor FXB+1\n    sta SGN\n    lda FXA+1\n    bpl cfm_apos\n    sec\n    lda #$00\n    sbc FXA\n    sta FXA\n    lda #$00\n    sbc FXA+1\n    sta FXA+1\ncfm_apos:\n    lda FXB+1\n    bpl cfm_bpos\n    sec\n    lda #$00\n    sbc FXB\n    sta FXB\n    lda #$00\n    sbc FXB+1\n    sta FXB+1\ncfm_bpos:");
    umul16();
    /* product >> 8 = 8.8 result, then apply the sign */
    asm("    lda PROD+1\n    sta FXR\n    lda PROD+2\n    sta FXR+1\n    bit SGN\n    bpl cfm_done\n    sec\n    lda #$00\n    sbc FXR\n    sta FXR\n    lda #$00\n    sbc FXR+1\n    sta FXR+1\ncfm_done:");
}

/* --- udiv24: DVD(24u) / DVS(16u) -> quotient DVD, remainder REM - */
void udiv24(void) {
    asm("    lda #$00\n    sta REM\n    sta REM+1\n    ldx #24\ncud_loop:\n    asl DVD\n    rol DVD+1\n    rol DVD+2\n    rol REM\n    rol REM+1\n    bcs cud_force\n    lda REM+1\n    cmp DVS+1\n    bcc cud_next\n    bne cud_force\n    lda REM\n    cmp DVS\n    bcc cud_next\ncud_force:\n    lda REM\n    sec\n    sbc DVS\n    sta REM\n    lda REM+1\n    sbc DVS+1\n    sta REM+1\n    inc DVD\ncud_next:\n    dex\n    bne cud_loop");
}

/* --- fdiv: FXR = FXA / FXB (signed 8.8) -------------------------
   Computes (|a| << 8) / |b|, then applies the sign. Saturates to
   +/-$7fff on overflow or division by zero. */
void fdiv(void) {
    /* result sign, DVD = |FXA| << 8, DVS = |FXB| */
    asm("    lda FXA+1\n    eor FXB+1\n    sta SGN\n    lda #$00\n    sta DVD\n    lda FXA\n    sta DVD+1\n    lda FXA+1\n    sta DVD+2\n    bpl cfd_apos\n    sec\n    lda #$00\n    sbc DVD+1\n    sta DVD+1\n    lda #$00\n    sbc DVD+2\n    sta DVD+2\ncfd_apos:\n    lda FXB\n    sta DVS\n    lda FXB+1\n    sta DVS+1\n    bpl cfd_bpos\n    sec\n    lda #$00\n    sbc DVS\n    sta DVS\n    lda #$00\n    sbc DVS+1\n    sta DVS+1\ncfd_bpos:");
    udiv24();
    /* saturate on overflow, else take the quotient; apply sign */
    asm("    lda DVD+2\n    beq cfd_ok\n    lda #$ff\n    sta FXR\n    lda #$7f\n    sta FXR+1\n    jmp cfd_sign\ncfd_ok:\n    lda DVD\n    sta FXR\n    lda DVD+1\n    sta FXR+1\ncfd_sign:\n    bit SGN\n    bpl cfd_done\n    sec\n    lda #$00\n    sbc FXR\n    sta FXR\n    lda #$00\n    sbc FXR+1\n    sta FXR+1\ncfd_done:");
}

/* --- isqrt24: SQROOT = floor(sqrt(SQN)), SQN 24-bit unsigned ----
   Binary digit-by-digit method, 12 iterations. Clobbers DVD. */
void isqrt24(void) {
    asm("    lda #$00\n    sta SQROOT\n    sta SQROOT+1\n    sta SQREM\n    sta SQREM+1\n    ldx #12\ncisq_loop:\n    asl SQN\n    rol SQN+1\n    rol SQN+2\n    rol SQREM\n    rol SQREM+1\n    asl SQN\n    rol SQN+1\n    rol SQN+2\n    rol SQREM\n    rol SQREM+1\n    asl SQROOT\n    rol SQROOT+1\n    lda SQROOT\n    asl\n    ora #$01\n    sta DVD\n    lda SQROOT+1\n    rol\n    sta DVD+1\n    lda SQREM+1\n    cmp DVD+1\n    bcc cisq_next\n    bne cisq_sub\n    lda SQREM\n    cmp DVD\n    bcc cisq_next\ncisq_sub:\n    lda SQREM\n    sec\n    sbc DVD\n    sta SQREM\n    lda SQREM+1\n    sbc DVD+1\n    sta SQREM+1\n    inc SQROOT\ncisq_next:\n    dex\n    bne cisq_loop");
}

/* --- fsqrt: FXR = sqrt(FXA), FXA is 8.8 and must be >= 0 --------
   sqrt(raw * 256) is exactly the 8.8 raw result. */
void fsqrt(void) {
    asm("    lda #$00\n    sta SQN\n    lda FXA\n    sta SQN+1\n    lda FXA+1\n    sta SQN+2");
    isqrt24();
    asm("    lda SQROOT\n    sta FXR\n    lda SQROOT+1\n    sta FXR+1");
}
