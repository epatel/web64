# Plan: convert raytracer-c's .asm files to C

Goal: move `lib/fixmath.asm`, `lib/gfx.asm`, `trace.asm` (and the render.asm
remnants) into C files, in the established style: C owns structure — function
decomposition, call graph, byte-global state, bit-test branching — while
inline `asm()` blocks hold what v0.1 cannot express (arithmetic, 16-bit ops,
loops via label scaffolds, table indexing). Verification anchor: the render
is deterministic, so after every phase the frame must stay pixel-identical.

Honest end-state: the arithmetic itself stays in asm strings until the
compiler grows expressions/loops. What we gain now is C-level structure,
runtime-tunable state, and files that migrate to real C statement-by-
statement as v0.1 becomes v0.x.

## Ground rules (from verified v0.1 behavior — see CLAUDE.md pitfalls)
- Zero-page interfaces stay (FXA/FXB/FXR, PX/PY/SHADE, GPX/GPY): perf is
  fmul-bound and zp is why; C can't declare zp, but asm blocks address it.
- All 16-bit ops in asm; `uint16_t` C globals are storage only.
- One string literal per `asm()`; `jmp` trampolines for loop-backs.
- Labels inside asm blocks are global: keep per-function prefixes
  (`fxi_`, `ua_`, `sq_`, `tp_` …) exactly as the asm originals do.
- Converted files break their symlinks (project gets its own copy);
  `projects/fixmath` and `projects/raytracer` remain the asm source of
  truth. Note divergence in both CLAUDE.mds when it happens.

## Phase 0 — spikes ✅ DONE (2026-07-13, all three passed, border green)
1. ✅ Multi-module C: `main.c` + `fixmath.c` + `selftest.c` + two headers
   build together; cross-file C→C calls work (`main` → `fx_selftest` →
   `umul16_c`). Prototypes in headers accepted; include guards work.
2. ✅ Self-modifying code in inline asm: `umul16` ported verbatim as
   `umul16_c()` in `fixmath.c` (labels renamed ua*→cua*, ub*→cub* to
   coexist with the lib copy during transition); operand patches execute
   correctly from the C module, product verified exact.
3. ✅ Self-test harness: `selftest.c` / `fx_selftest()` checks umul16_c,
   fmul, fdiv, fsqrt against exact raw values; toggle block in `main()`
   (border green/red). Render path restored and compiling after.

## Phase 1 — fixmath.c ✅ DONE (2026-07-13)
All of fx_init, umul16, fmul, udiv24, fdiv, isqrt24, fsqrt ported to
`fixmath.c` (internal labels prefixed c*). C owns the call graph — fmul
calls umul16(), fdiv calls udiv24(), fsqrt calls isqrt24() as C calls.
`lib/fixmath.asm` removed from the project; its zp/table equates moved to
render.asm, which also carries transition bridges for trace.asm callers
(`fmul: jmp _fmul`, fdiv, fsqrt — removed in Phase 3). Verified: self-test
green with the asm lib gone, full-frame golden image identical.

## Phase 2 — gfx.c ✅ DONE (2026-07-13)
`gfx_init` is real C (VIC writes via c64.h pointer constants; only the
CIA2 bank-select read-modify-write stays asm — no |= in v0.1);
`gfx_clear`/`gfx_plot` are asm-scaffold C functions (labels cg*).
Tables (gfx_bits, ytab_lo/hi) and GPTR/GPX/GPY equates moved into
render.asm rather than a separate gfxdata.asm — it already holds the
dither tables. `gfx_line`/`gfx_circle` dropped (unused; fixmath project
keeps the asm originals). Bonus: the render loop's plot decision became
a C `if (do_plot & 1)` with `gfx_plot()` as a real C call, so no gfx
bridges were ever needed. `lib/gfx.asm` removed from the build and the
`lib/` symlink deleted — the project is now standalone. Verified:
golden image identical.

## Phase 3 — trace.c ✅ DONE (2026-07-13)
`trace_pixel` decomposed into C functions: ray_setup, sphere_hit_point,
diffuse_light, mirror_bounce, primary_ray, sky_gradient, floor_sample,
floor_hit_point, shadow_test, floor_checker, sphere_shade. All branching
is C ifs on byte-global flags (sph_hit, lit, sky_ray, far_floor,
shad_ok, in_shadow) set by asm compares; the shadow test's early-outs
became FLAT sequential guards on shad_ok (no if-nesting needed). All
fixmath calls are C calls, so the render.asm bridges are gone. The
.word scratch registers moved to render.asm (v0.1 word globals can't be
manipulated at runtime). trace.asm symlink deleted; scene.asm stays
(equates referenced by name from asm blocks). Verified: C calls inside
if bodies work; golden image identical. Stretch goal (scene constants
as C globals) not taken — left for later if wanted.

## Phase 4 — render.asm remnants + cleanup ✅ DONE (2026-07-13)
`noise_init` moved to main.c (LFSR loop as one asm scaffold); the last
asm_* wrapper was already gone. render.asm now contains ZERO code —
only zp equates, .word scratch registers, and data tables. Verified:
mode-1 white-noise spot render (exercises the C noise_init's ntab) plus
the mode-2 golden image, both correct.

## PORT COMPLETE — final state
Executable code: 100% C modules (main.c, trace.c, fixmath.c, gfx.c,
selftest.c). Assembly that remains, all non-executable, all things
Web64 C v0.1 fundamentally cannot hold:
- render.asm: zero-page equates, 16-bit scratch registers, data tables
- scene.asm (symlink): scene constant equates
Future compiler versions can migrate the asm() block contents to real C
statement-by-statement; the structure is already in place.

Follow-up ✅ DONE (2026-07-13): scene constants lifted to C globals
(scene.c/scene.h, scene.asm symlink removed) — trace.c's asm blocks
load them from memory at runtime, so the scene is tweakable without
reassembling the kernel. Golden image identical. The project is fully
standalone: no symlinks, no asm sources except the render.asm data
module.

## Risks / open questions
- Multi-module C build and C→C cross-file calls: unverified (Phase 0.1).
- Self-modifying asm inside a C module: unverified (Phase 0.2); fallback —
  umul16 stays in a tiny .asm file, everything else still converts.
- Code growth: C lowering is verbose; watch branch ranges (trampolines)
  and total size — code starts at $4000, must stay below fx tables at
  $c000 (~48KB headroom, plenty).
- Perf: zp interfaces preserved and hot loops stay asm-shaped, so the
  frame time should hold at ~2:20 warp; if a phase regresses it, the
  golden-image run will also time it — investigate before proceeding.
- IDE iteration: use the CORS-server + textarea-injection workflow per
  file; regenerate the .web64proj only at phase ends.
