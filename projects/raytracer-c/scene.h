#ifndef RAYTRACER_C_SCENE_H
#define RAYTRACER_C_SCENE_H

#include <stdint.h>

/* Scene definition — see scene.c. All values are raw signed 8.8.
   The trace.c asm blocks read these at RUNTIME (_sph_cy, ...), so
   the scene is tweakable without touching the kernel — but the
   derived constants must be kept in sync by hand (formulas in
   scene.c). */

extern uint16_t sph_cy, sph_cz, sph_c2r;
extern uint16_t lgt_x, lgt_y, lgt_z;
extern uint16_t ocy_ly, cc_sh;
extern uint16_t floor_y;
extern uint8_t shadow_max;
extern uint8_t shd_chk_hi, shd_chk_lo, shd_haze, sky_max;

#endif
