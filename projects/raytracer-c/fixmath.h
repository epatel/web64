#ifndef RAYTRACER_C_FIXMATH_H
#define RAYTRACER_C_FIXMATH_H

/* Phase 0 spike: umul16 ported verbatim into a C module (self-
   modifying code inside asm()). Zero-page interface unchanged:
   PROD(32u) = MULA(16u) * MULB(16u), requires fx_init. */
void umul16_c(void);

#endif
