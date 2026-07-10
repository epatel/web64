# fixmath — 8.8 fixed-point math + hires gfx library

Project 1 of 2 (raytracer prelude). `lib/` here is the **single source of
truth**; the raytracer project symlinks to it. Change the lib here, and both
projects pick it up.

## Files
- `lib/fixmath.asm` — signed 8.8 fixed point: `fx_init`, `umul16`, `fmul`,
  `fdiv` (saturating), `isqrt24`, `fsqrt`
- `lib/gfx.asm` — 320x200 hires bitmap at `$2000`: `gfx_init`, `gfx_clear`,
  `gfx_plot`, `gfx_line` (Bresenham), `gfx_circle` (midpoint)
- `main.asm` — demo/test harness: frame + line fan + two circles, plus a
  numeric self-test → border GREEN = pass, RED = fail

## Critical conventions
- **`jsr fx_init` once at startup** before any multiply — it builds the
  quarter-square tables (`f(x)=x²/4`) at `$c000-$c7ff`. Without it, all
  multiplies return garbage.
- `umul16` is self-modifying (patches operand bytes into table lookups):
  ~275 cycles, preserves `MULA`/`MULB`. Safe to call repeatedly; `fx_init`
  is also idempotent.
- Zero page: lib owns `$57-$6f` (BASIC FAC area — safe unless calling BASIC
  float routines); gfx owns `$fb-$fe` + `$02`. `fmul` destroys `FXA`/`FXB`.
- 8.8 range is ±127.996: products of intermediate values must stay under 128
  or they silently wrap (`fdiv` saturates, `fmul` does not).
- Memory map: bitmap `$2000-$3f3f`, code at `$4000`, tables `$c000-$c7ff`.

## Testing
Load `main.asm` (flattened or with the virtual tree) in the web64 IDE, run:
green border + clean circles/lines = lib is good. The self-test checks
2.5×4.0, −10.0/4.0 and √2.25 for exact expected raw values.
