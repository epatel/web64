#ifndef RAYTRACER_C_TRACE_H
#define RAYTRACER_C_TRACE_H

/* Per-pixel ray tracing (C port of trace.asm — see trace.c).
   Input: PX ($03/$04, 0-319), PY ($05, 0-199).
   Output: SHADE ($06, 0-15). Scene constants from scene.asm. */

void trace_pixel(void);

#endif
