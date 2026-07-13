# raytracer-c — the raytracer with a Web64 C v0.1 front end

Same image as `projects/raytracer` (mirror sphere, checkered floor, shadows,
sky gradient, animation-stable dithering), but the entry point, startup
sequence, and runtime tuning live in C. Web64 C v0.1 has no loops, arrays,
or general expressions, so the render loop and 8.8 math kernel stay in
assembly — this is the mixed C/assembly boundary the IDE manual recommends,
not a limitation we chose.

## Files
- `main.c` — C entry (`main` → `_main`): `sei`, init calls, render, halt.
  Owns the tuning globals `dither_mode` (0 Bayer / 1 white noise / 2 blue
  noise) and `noise_seed`, which the assembly reads at RUNTIME as
  `_dither_mode` / `_noise_seed` — unlike the pure-asm raytracer, changing
  dither mode needs no reassembly of the kernel.
- `render.asm` — render loop, dither tables, `asm_*` wrappers
  (`asm_fx_init`, `asm_gfx_init`, `asm_noise_init`, `asm_render`) that C
  calls via `jsr`. `.include`s everything below. No org: code flows after
  the generated C module from the project origin.
- `scene.asm`, `trace.asm` — **symlinks to `../raytracer/`** (source of
  truth; kernel is byte-identical)
- `lib/` — **symlink to `../fixmath/lib`** (source of truth)

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
Local clangd flags `main.c` (`'c64.h' file not found`, implicit `asm_*`
declarations, `main` return type). All expected: `c64.h` is a Web64 virtual
bundled header, and implicit `asm_*` calls are the documented Web64 C
pattern. Judge the code by the IDE's Diagnostics panel, not clangd.

## Testing
Run in the web64 IDE with **warp on**. Deterministic dither: a correct
build renders pixel-identical to `projects/raytracer` at the same
`dither_mode` — compare screenshots against that project to detect
regressions. Frame ~2:20 under warp.
