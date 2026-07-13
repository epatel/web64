#ifndef RAYTRACER_C_GFX_H
#define RAYTRACER_C_GFX_H

/* C port of the gfx lib — 320x200 hires bitmap at $2000, screen
   RAM $0400, VIC bank 0. Zero page: GPTR $fb/$fc, GPX $fd/$fe,
   GPY $02 (equates and tables in render.asm).
   gfx_line/gfx_circle are not ported: the raytracer never calls
   them (projects/fixmath keeps the asm originals). */

void gfx_init(void);   /* bitmap on, cleared, white on black, y-table built */
void gfx_clear(void);  /* clear bitmap + reset colors */
void gfx_plot(void);   /* set pixel at GPX (0-319), GPY (0-199) */

#endif
