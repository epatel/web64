/* ---------------------------------------------------------------
   RAYTRACER-C — Web64 C v0.1 edition of the raytracer.

   C owns the entry point, the startup sequence, and the runtime
   tuning globals below. Everything with loops or 8.8 fixed-point
   math (render loop, trace kernel, fixmath, gfx) stays in
   assembly — Web64 C v0.1 has no loops, arrays, or general
   expressions, so the kernel is not expressible in the subset.

   C globals are exported with underscore labels: dither_mode is
   _dither_mode in assembly, read per pixel by render.asm.
   asm_* calls lower to `jsr asm_*`, defined in render.asm.
   --------------------------------------------------------------- */

#include <c64.h>
#include <stdint.h>

/* Dither mode, read at runtime (the pure-asm raytracer fixes this
   at assemble time). 0 = ordered 4x4 Bayer, 1 = seeded white
   noise, 2 = 16x16 blue noise (default). All are pure functions
   of (x, y): animation-stable, identical image on every run. */
uint8_t dither_mode = 2;

/* White-noise LFSR seed, mode 1 only; any nonzero byte. */
uint8_t noise_seed = 0x5a;

void main(void) {
    asm("sei");
    asm_fx_init();     /* quarter-square tables at $c000 — required */
    asm_gfx_init();    /* hires bitmap at $2000, cleared            */
    asm_noise_init();  /* LFSR noise table from _noise_seed         */
    asm_render();      /* trace + dither all 320x200 pixels         */
    asm("halt:\n    jmp halt");
}
