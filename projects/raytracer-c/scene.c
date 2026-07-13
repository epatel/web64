/* ---------------------------------------------------------------
   scene.c — raytracer scene definition (raw signed 8.8 values).

   One mirror sphere, directional light, checkered floor, sky.
   C port of scene.asm: the constants are now C globals read at
   runtime by the trace.c asm blocks, so the scene can be tuned
   (or animated) without reassembling the kernel.

   Derived constants MUST be kept in sync by hand (v0.1 C cannot
   multiply, and the assembler never could):
     sph_c2r = cy*cy + cz*cz - r*r
     ocy_ly  = (floor_y - sph_cy) * lgt_y
     cc_sh   = (floor_y - sph_cy)^2 - r*r
   --------------------------------------------------------------- */

#include "scene.h"

/* sphere: center (0, sph_cy, sph_cz), radius 1.0 */
uint16_t sph_cy = 0x0080;   /*  0.5 (floating above the floor) */
uint16_t sph_cz = 0x0200;   /*  2.0 */
uint16_t sph_c2r = 0x0340;  /*  3.25 = 0.25 + 4 - 1 */

/* directional light, unit vector pointing toward the light */
uint16_t lgt_x = 0xff98;    /* -0.408 */
uint16_t lgt_y = 0x00d1;    /*  0.816 */
uint16_t lgt_z = 0xff98;    /* -0.408 */

/* shadow-ray constants (ocy = floor_y - sph_cy = -1.5) */
uint16_t ocy_ly = 0xfec7;   /* ocy * lgt_y = -1.224 */
uint16_t cc_sh = 0x0140;    /* ocy*ocy - r*r = 1.25 */
uint8_t shadow_max = 6;     /* shadow-test only within +/-6 units (8.8 range) */

/* floor and shading */
uint16_t floor_y = 0xff00;  /* -1.0 */
uint8_t shd_chk_hi = 13;    /* bright checker tile */
uint8_t shd_chk_lo = 3;     /* dark checker tile */
uint8_t shd_haze = 1;       /* distant floor / horizon band */
uint8_t sky_max = 12;       /* sky shade looking straight up */
