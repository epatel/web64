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

   All callers are C (trace.c, selftest.c) — direct C calls.
   --------------------------------------------------------------- */

#include "fixmath.h"

/* --- fx_init: build the quarter-square tables (call once) -------
   f(n) = floor(n*n/4), built incrementally: f(n+1) = f(n) +
   floor((n+1)/2). SQR2[i] = f(|i-255|) is copied from SQR1.
   Uses SQN/SQROOT as pointers and REM/DVS as accumulator/delta. */
void fx_init(void) {
    asm("
        lda #<SQR1LO
        sta SQN
        lda #>SQR1LO
        sta SQN+1
        lda #<SQR1HI
        sta SQROOT
        lda #>SQR1HI
        sta SQROOT+1
        lda #$00
        sta REM
        sta REM+1
        sta DVS
        tay
        ldx #$02
    cfxi_loop:
        lda REM
        sta (SQN),y
        lda REM+1
        sta (SQROOT),y
        tya
        and #$01
        beq cfxi_add
        inc DVS
    cfxi_add:
        clc
        lda REM
        adc DVS
        sta REM
        lda REM+1
        adc #$00
        sta REM+1
        iny
        bne cfxi_loop
        inc SQN+1
        inc SQROOT+1
        dex
        bne cfxi_loop
        ldx #$00
        ldy #$ff
    cfxi_c1:
        lda SQR1LO,y
        sta SQR2LO,x
        lda SQR1HI,y
        sta SQR2HI,x
        dey
        inx
        bne cfxi_c1
    cfxi_c2:
        lda SQR1LO+1,x
        sta SQR2LO+$100,x
        lda SQR1HI+1,x
        sta SQR2HI+$100,x
        inx
        bne cfxi_c2
    ");
}

/* --- umul16: PROD = MULA * MULB (unsigned 16x16 -> 32) ----------
   Quarter-square multiply, four 8x8 partial products. SELF-
   MODIFYING: the operand low bytes are patched into the table
   addresses (`sta cua1+1` rewrites the lda below). Verbatim from
   the asm lib. Requires fx_init. Preserves MULA/MULB. */
void umul16(void) {
    asm("
        lda MULA
        sta cua1+1
        sta cua2+1
        sta cua5+1
        sta cua6+1
        eor #$ff
        sta cua3+1
        sta cua4+1
        sta cua7+1
        sta cua8+1
        lda MULA+1
        sta cub1+1
        sta cub2+1
        sta cub5+1
        sta cub6+1
        eor #$ff
        sta cub3+1
        sta cub4+1
        sta cub7+1
        sta cub8+1
        ldy MULB
        sec
    cua1:    lda SQR1LO,y
    cua3:    sbc SQR2LO,y
        sta PROD
    cua2:    lda SQR1HI,y
    cua4:    sbc SQR2HI,y
        sta PROD+1
        sec
    cub1:    lda SQR1LO,y
    cub3:    sbc SQR2LO,y
        sta UT2
    cub2:    lda SQR1HI,y
    cub4:    sbc SQR2HI,y
        sta UT2+1
        ldy MULB+1
        sec
    cua5:    lda SQR1LO,y
    cua7:    sbc SQR2LO,y
        sta UT1
    cua6:    lda SQR1HI,y
    cua8:    sbc SQR2HI,y
        sta UT1+1
        sec
    cub5:    lda SQR1LO,y
    cub7:    sbc SQR2LO,y
        sta PROD+2
    cub6:    lda SQR1HI,y
    cub8:    sbc SQR2HI,y
        sta PROD+3
        clc
        lda UT1
        adc UT2
        sta UT1
        lda UT1+1
        adc UT2+1
        sta UT1+1
        lda #$00
        adc #$00
        sta UT3
        clc
        lda PROD+1
        adc UT1
        sta PROD+1
        lda PROD+2
        adc UT1+1
        sta PROD+2
        lda PROD+3
        adc UT3
        sta PROD+3
    ");
}

/* --- fmul: FXR = FXA * FXB (signed 8.8) ------------------------- */
void fmul(void) {
    /* result sign, |FXA|, |FXB| */
    asm("
        lda FXA+1
        eor FXB+1
        sta SGN
        lda FXA+1
        bpl cfm_apos
        sec
        lda #$00
        sbc FXA
        sta FXA
        lda #$00
        sbc FXA+1
        sta FXA+1
    cfm_apos:
        lda FXB+1
        bpl cfm_bpos
        sec
        lda #$00
        sbc FXB
        sta FXB
        lda #$00
        sbc FXB+1
        sta FXB+1
    cfm_bpos:
    ");
    umul16();
    /* product >> 8 = 8.8 result, then apply the sign */
    asm("
        lda PROD+1
        sta FXR
        lda PROD+2
        sta FXR+1
        bit SGN
        bpl cfm_done
        sec
        lda #$00
        sbc FXR
        sta FXR
        lda #$00
        sbc FXR+1
        sta FXR+1
    cfm_done:
    ");
}

/* --- udiv24: DVD(24u) / DVS(16u) -> quotient DVD, remainder REM - */
void udiv24(void) {
    asm("
        lda #$00
        sta REM
        sta REM+1
        ldx #24
    cud_loop:
        asl DVD
        rol DVD+1
        rol DVD+2
        rol REM
        rol REM+1
        bcs cud_force
        lda REM+1
        cmp DVS+1
        bcc cud_next
        bne cud_force
        lda REM
        cmp DVS
        bcc cud_next
    cud_force:
        lda REM
        sec
        sbc DVS
        sta REM
        lda REM+1
        sbc DVS+1
        sta REM+1
        inc DVD
    cud_next:
        dex
        bne cud_loop
    ");
}

/* --- fdiv: FXR = FXA / FXB (signed 8.8) -------------------------
   Computes (|a| << 8) / |b|, then applies the sign. Saturates to
   +/-$7fff on overflow or division by zero. */
void fdiv(void) {
    /* result sign, DVD = |FXA| << 8, DVS = |FXB| */
    asm("
        lda FXA+1
        eor FXB+1
        sta SGN
        lda #$00
        sta DVD
        lda FXA
        sta DVD+1
        lda FXA+1
        sta DVD+2
        bpl cfd_apos
        sec
        lda #$00
        sbc DVD+1
        sta DVD+1
        lda #$00
        sbc DVD+2
        sta DVD+2
    cfd_apos:
        lda FXB
        sta DVS
        lda FXB+1
        sta DVS+1
        bpl cfd_bpos
        sec
        lda #$00
        sbc DVS
        sta DVS
        lda #$00
        sbc DVS+1
        sta DVS+1
    cfd_bpos:
    ");
    udiv24();
    /* saturate on overflow, else take the quotient; apply sign */
    asm("
        lda DVD+2
        beq cfd_ok
        lda #$ff
        sta FXR
        lda #$7f
        sta FXR+1
        jmp cfd_sign
    cfd_ok:
        lda DVD
        sta FXR
        lda DVD+1
        sta FXR+1
    cfd_sign:
        bit SGN
        bpl cfd_done
        sec
        lda #$00
        sbc FXR
        sta FXR
        lda #$00
        sbc FXR+1
        sta FXR+1
    cfd_done:
    ");
}

/* --- isqrt24: SQROOT = floor(sqrt(SQN)), SQN 24-bit unsigned ----
   Binary digit-by-digit method, 12 iterations. Clobbers DVD. */
void isqrt24(void) {
    asm("
        lda #$00
        sta SQROOT
        sta SQROOT+1
        sta SQREM
        sta SQREM+1
        ldx #12
    cisq_loop:
        asl SQN
        rol SQN+1
        rol SQN+2
        rol SQREM
        rol SQREM+1
        asl SQN
        rol SQN+1
        rol SQN+2
        rol SQREM
        rol SQREM+1
        asl SQROOT
        rol SQROOT+1
        lda SQROOT
        asl
        ora #$01
        sta DVD
        lda SQROOT+1
        rol
        sta DVD+1
        lda SQREM+1
        cmp DVD+1
        bcc cisq_next
        bne cisq_sub
        lda SQREM
        cmp DVD
        bcc cisq_next
    cisq_sub:
        lda SQREM
        sec
        sbc DVD
        sta SQREM
        lda SQREM+1
        sbc DVD+1
        sta SQREM+1
        inc SQROOT
    cisq_next:
        dex
        bne cisq_loop
    ");
}

/* --- fsqrt: FXR = sqrt(FXA), FXA is 8.8 and must be >= 0 --------
   sqrt(raw * 256) is exactly the 8.8 raw result. */
void fsqrt(void) {
    asm("
        lda #$00
        sta SQN
        lda FXA
        sta SQN+1
        lda FXA+1
        sta SQN+2
    ");
    isqrt24();
    asm("
        lda SQROOT
        sta FXR
        lda SQROOT+1
        sta FXR+1
    ");
}
