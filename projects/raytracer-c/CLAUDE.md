# raytracer-c — the raytracer with a Web64 C v0.1 front end

Same image as `projects/raytracer` (mirror sphere, checkered floor, shadows,
sky gradient, animation-stable dithering), but the entry point, startup
sequence, runtime tuning, AND the render loop live in C. Web64 C v0.1 has
no loop statements, arrays, arithmetic, or comparisons, so the render loop
in `main.c` scaffolds its control flow with inline `asm()` labels/branches
around lowered C statements (emission order is linear, so this works —
verified pixel-identical against the pure-asm raytracer). The 8.8 math
kernel (trace_pixel, fixmath, gfx) stays in assembly modules.

A macro utility header for the missing constructs is NOT possible:
function-like `#define` is not expanded at all, and object-like `#define`
expands only in expression position (constants). Only include guards and
constant defines survive preprocessing (all tested against the compiler).

## Files
- `main.c` — C entry (`main` → `_main`) and the C `render()` loop: loop
  counters `px`/`py` as C globals, dither mode dispatch as flat C bit-test
  `if`s (Bayer computed unconditionally, mode bits override), short inline
  asm blocks for zero-page marshalling, table lookups, compares, and loop
  branches. Owns the tuning globals `dither_mode` (0 Bayer / 1 white noise
  / 2 blue noise) and `noise_seed`, read at RUNTIME — changing dither mode
  needs no reassembly of the kernel.
- `render.asm` — pure data module, NO code (since Phase 4): zero-page
  equates (PX/PY/SHADE, fixmath registers, GPTR/GPX/GPY), the 16-bit
  trace scratch registers, and the data tables (bayer4, ntab, bnoise,
  gfx_bits, ytab). Everything executable is C; this file holds what
  v0.1 fundamentally cannot (zp, runtime 16-bit ops, arrays).
- `fixmath.c` / `fixmath.h` — the fixmath library in C (Phase 1 of
  PLAN-c-port.md complete): fx_init, umul16 (self-modifying, verbatim),
  fmul, udiv24, fdiv, isqrt24, fsqrt. Same zero-page interface (equates
  now in render.asm); trace.asm reaches fmul/fdiv/fsqrt via bridge
  labels in render.asm until Phase 3. `lib/fixmath.asm` is no longer in
  the build (the lib/ symlink remains for gfx.asm).
- `gfx.c` / `gfx.h` — the gfx lib in C (Phase 2): `gfx_init` is real C
  (VIC pointer writes), `gfx_clear`/`gfx_plot` asm-scaffold functions.
  `gfx_line`/`gfx_circle` intentionally not ported (unused here; the
  fixmath project keeps the asm originals). The render loop calls
  `gfx_plot()` as a C call behind a C `if (do_plot & 1)`.
- `selftest.c` / `selftest.h` — `fx_selftest()`: exact-value checks for
  umul16/fmul/fdiv/fsqrt into `selftest_ok`; enable via the toggle
  block in `main()` (border green = pass, red = fail). The fast smoke
  test while porting fixmath — seconds instead of a 2.5-minute render.
- `trace.c` / `trace.h` — the ray-tracing kernel in C (Phase 3):
  trace_pixel decomposed into ray_setup / sphere_hit_point /
  diffuse_light / mirror_bounce / primary_ray / sky_gradient /
  floor_sample / shadow_test / floor_checker / sphere_shade. Branching
  is C ifs on byte-global flags set by asm compares (shadow early-outs
  are flat sequential guards on `shad_ok`); the 8.8 math sequences are
  asm blocks calling the C fixmath. Scratch .word registers live in
  render.asm.
- `scene.c` / `scene.h` — the scene as C globals, read at RUNTIME by
  trace.c's asm blocks (`_sph_cy`, `_lgt_x`, ...): sphere, light,
  floor, shade levels are tweakable (or animatable) without touching
  the kernel. Derived constants (sph_c2r, ocy_ly, cc_sh) must still be
  recomputed by hand — formulas in the file header. All symlinks to
  the asm projects are gone; the project is fully standalone.

## Web64 C v0.1 pitfalls hit here (all verified in the IDE)
- `asm()` takes ONE string literal — adjacent-literal concatenation
  (`"a" "b"`) is not performed; fragments leak into the assembly output
  as quoted garbage. But RAW NEWLINES inside the literal are accepted
  (verified byte-identical to `\n` escapes), so blocks are written as
  multiline strings that read like normal assembly listings.
- `uint16_t` globals: the initializer emits a proper `.WORD`, but ALL
  runtime operations lower low-byte-only — `px++` doesn't carry into
  `_px+1`, `px = 0` doesn't clear `_px+1`. Do every 16-bit manipulation
  in inline asm; C only declares the storage.
- C-lowered loop bodies are long: backward branches easily exceed the
  6502's -128 range — use `jmp` trampolines for loop-back edges.
- `if (x & MASK) { ...asm()... }` works and keeps emission order; flat
  ifs with unconditional default beat if/else (no `else` in v0.1).

## How the mixed build works (verified in the IDE 2026-07-13)
- IDE setup: open `raytracer-c.web64proj` (generated from the repo sources;
  regenerate after changes — symlinked files are inlined into it, so the
  repo `.asm`/`.c` files stay the source of truth). Main source =
  `render.asm`; `main.c`, `scene.asm`, `trace.asm`, `lib/*.asm` are virtual
  files; the `"c"` block is enabled with entry `main.c`. Origin `$4000`.
- With C enabled, the build graph assembles EVERY project `.asm` file as a
  module automatically — do **not** `.include` sibling `.asm` files
  (duplicate-symbol diagnostics). One shared symbol namespace: C `main`
  becomes `_main`, C globals become `_name` labels, `asm_*()` calls emit
  `jsr asm_*`.
- The C runtime bootstrap defines `start:` at the origin and does
  `jsr _main` — so no asm file may define `start`, and entry auto-detect
  picks `start $4000`. The tiny runtime's `_web64_c_arg0` at `$fb` overlaps
  gfx `GPTR` — harmless while C stdio is never called.
- Memory map otherwise identical to the raytracer: bitmap `$2000-$3f3f`,
  code from `$4000`, fx tables built at `$c000` by `fx_init`, zero page
  PX/PY/SHADE at `$03-$06`, lib owns `$57-$6f` and `$fb-$fe`+`$02`.

## Editor noise
Local clangd flags the C files heavily: `'c64.h' file not found` (Web64
virtual bundled header), `main` return type, and — loudest — "Expected
string literal in 'asm' / missing terminating '"'" on every multiline
asm() block (ISO C forbids raw newlines in string literals; Web64 C
accepts them). All expected. Judge the code by the IDE's Diagnostics
panel, not clangd.

## Testing
Run in the web64 IDE with **warp on**. Deterministic dither: a correct
build renders pixel-identical to `projects/raytracer` at the same
`dither_mode` — compare screenshots against that project to detect
regressions. Frame ~2:20 under warp.
