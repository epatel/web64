# raytracer — mirror sphere over a checkered floor

Project 2 of 2, consumes the fixmath lib. Renders a shiny white
(mirror + Lambertian) sphere floating over an infinite checkerboard,
directional light with cast shadows, sky darkening toward the horizon.
Dithered to 320x200 hires mono.

## Files
- `main.asm` — entry, render loop, dither modes, noise/blue-noise tables,
  `.include`s everything else
- `scene.asm` — ALL scene tuning lives here (sphere, light, shades).
  Derived constants (`SPH_C2R`, `OCY_LY`, `CC_SH`) must be recomputed by
  hand when the sphere or light moves — formulas are in the file header.
- `trace.asm` — `trace_pixel`: half-b sphere quadratic, one-bounce mirror
  reflection, N·L diffuse, floor shadow ray, sky gradient. Algorithm
  documented in the file header.
- `lib/` — **symlink to `../fixmath/lib`** (fixmath is the source of truth)
- `tools/bluenoise.py` — regenerates the blue-noise table (void-and-cluster,
  deterministic; `python3 bluenoise.py [seed]` → paste the `.byte` rows)
- `raytracer.web64proj` — saved IDE project snapshot (refresh after changes)

## Key facts
- Entry `start` at `$4000`; `jsr fx_init` must run before any math
- Dithering: `DITHER_MODE` equate — 0 Bayer, 1 seeded white noise,
  2 blue noise (default). All are pure functions of (x, y), so animation
  never flickers: the pattern stays glued to the screen.
- Camera at origin looking +z, screen plane z=1; ray dirs left unnormalized
  (the math tolerates it — saves fsqrt+fdiv per pixel)
- 8.8 overflow guards: floor sampling clamps t2 ≥ 32 to a haze shade;
  shadow tests only run within ±6 units (`SHADOW_MAX`) so squares fit
- Frame time ~2:20 under warp. Cost is fmul-dominated; next optimization
  targets: reciprocal-table fdiv, per-scanline constant folding.

## Testing
Run in the web64 IDE with **warp on** (Emulator Options → RUNTIME → Warp).
Because the dither is deterministic, any correct build renders a
pixel-identical image — compare screenshots to detect regressions.
Warp hogs the tab's main thread: pause or drop to 1x before using IDE UI
(e.g. Save Project), or clicks may be swallowed.
