#ifndef RAYTRACER_C_FIXMATH_H
#define RAYTRACER_C_FIXMATH_H

/* C port of the fixmath lib — signed 8.8 fixed point.
   All values pass through zero page (equates in render.asm):
     FXA/FXB $57/$59 operands, FXR $6e result,
     MULA/MULB/PROD for umul16, SQN/SQROOT for isqrt24.
   fx_init MUST run once before any multiply. */

void fx_init(void);   /* build quarter-square tables at $c000 */
void umul16(void);    /* PROD = MULA * MULB (16x16 -> 32, unsigned) */
void fmul(void);      /* FXR = FXA * FXB (destroys FXA/FXB) */
void udiv24(void);    /* DVD = DVD / DVS, remainder REM (internal) */
void fdiv(void);      /* FXR = FXA / FXB (saturating) */
void isqrt24(void);   /* SQROOT = floor(sqrt(SQN)) (internal) */
void fsqrt(void);     /* FXR = sqrt(FXA), FXA >= 0 */

#endif
