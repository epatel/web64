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
- `render.asm` — what the C subset genuinely cannot hold: PX/PY/SHADE
  zero-page equates, `asm_fx_init`/`asm_gfx_init` wrappers,
  `asm_noise_init` (LFSR needs shifts/carry), and the dither tables
  (bayer4, ntab, bnoise, BAYIX, DTH). No org: code flows after the
  generated C module from the project origin.
- `fixmath.c` / `fixmath.h` — the fixmath library in C (Phase 1 of
  PLAN-c-port.md complete): fx_init, umul16 (self-modifying, verbatim),
  fmul, udiv24, fdiv, isqrt24, fsqrt. Same zero-page interface (equates
  now in render.asm); trace.asm reaches fmul/fdiv/fsqrt via bridge
  labels in render.asm until Phase 3. `lib/fixmath.asm` is no longer in
  the build (the lib/ symlink remains for gfx.asm).
- `selftest.c` / `selftest.h` — `fx_selftest()`: exact-value checks for
  umul16_c/fmul/fdiv/fsqrt into `selftest_ok`; enable via the toggle
  block in `main()` (border green = pass, red = fail). The fast smoke
  test while porting fixmath — seconds instead of a 2.5-minute render.
- `scene.asm`, `trace.asm` — **symlinks to `../raytracer/`** (source of
  truth; kernel is byte-identical)
- `lib/` — **symlink to `../fixmath/lib`** (source of truth; files break
  the symlink as their C port lands — see PLAN-c-port.md)

## Web64 C v0.1 pitfalls hit here (all verified in the IDE)
- `asm()` takes ONE string literal — adjacent-literal concatenation
  (`"a" "b"`) is not performed; fragments leak into the assembly output
  as quoted garbage. Write each block as a single string with `\n`.
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
Local clangd flags `main.c` (`'c64.h' file not found`, implicit `asm_*`
declarations, `main` return type). All expected: `c64.h` is a Web64 virtual
bundled header, and implicit `asm_*` calls are the documented Web64 C
pattern. Judge the code by the IDE's Diagnostics panel, not clangd.

## Testing
Run in the web64 IDE with **warp on**. Deterministic dither: a correct
build renders pixel-identical to `projects/raytracer` at the same
`dither_mode` — compare screenshots against that project to detect
regressions. Frame ~2:20 under warp.
