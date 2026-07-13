#ifndef RAYTRACER_C_SELFTEST_H
#define RAYTRACER_C_SELFTEST_H

#include <stdint.h>

/* Numeric self-test (port of projects/fixmath main.asm fx_selftest,
   plus a umul16_c check). Sets selftest_ok to 1 on pass, 0 on any
   failure. Fast smoke test for the fixmath C port — seconds instead
   of a full render. Requires fx_init. */
extern uint8_t selftest_ok;
void fx_selftest(void);

#endif
