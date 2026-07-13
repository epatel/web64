# c-compiler

Web64 C v0.1: the experimental browser-native C compiler, its supported subset, bundled
headers (`c64.h` etc.), configuration, and mixed C/assembly projects.

Web64 C is NOT cc65 — no object files, linker scripts, or native toolchain. Supported C
modules are lowered to Web64 assembler-compatible source; the existing browser-local
assembler then produces the PRG, symbols, line maps, diagnostics, and memory ranges.
Intended for small C64 programs, register setup, asset-driven game code, and mixed
C/assembly projects.

## Project model

`.c` files are C modules, `.h` files are C headers — ordinary virtual project files,
saved inside `.web64proj` alongside everything else. C modules are ordered before
assembly modules so generated C labels and handwritten assembly labels share one
assembler symbol namespace.

- C names get underscore labels: C `main` becomes `_main`; a global `player_x` becomes
  `_player_x`. Handwritten assembly can call/reference these.
- Calls to names starting with `asm_` emit `jsr asm_name` directly — define
  `asm_name:` in a project assembly file.

## Configuration (`.web64proj` `c` block, project-relative paths only)

```json
{
  "c": {
    "enabled": true,
    "dialect": "web64-c-v0.1",
    "entry": "main.c",
    "includePaths": ["include", "assets"],
    "stdout": "screen",
    "stdin": "keyboard",
    "runtimeProfile": "tiny",
    "compilerBackend": "web64-native"
  }
}
```

stdout/stdin backends: `screen`, `kernal`, `debug`, `none` (use `none` for freestanding
routines that don't need the tiny runtime I/O shims). `runtimeProfile`: `tiny` or
`freestanding`. Default config has `"enabled": false`.

## Supported v0.1 subset (narrow — unsupported statements produce diagnostics)

- Globals: `uint8_t`, `char`, `uint16_t`, `int`, `unsigned int` with constant
  initializers; byte-sized structs with global instances.
- Functions returning `void`/`int`/`char`/`uint8_t`/`uint16_t`/`unsigned char`/
  `unsigned int`; direct calls; `return` with optional constant byte value.
- Assignments to globals, increments/decrements.
- Pointer reads/writes via `*(volatile uint8_t *)ADDRESS`.
- Bit tests: `if (value & MASK)` / `if (!(value & MASK))` (joystick-style active-low).
- Inline assembly: `asm("...")` / `__asm__("...")` — emitted directly into the generated
  assembly module; keep labels unique if the block can be emitted more than once.
- `#include` of bundled and project-local virtual headers.
- Numbers in decimal, `0x` hex, and `$` hex.

NOT supported: complex expressions, arrays, loops, nested/general control flow, full
preprocessing, full libc, broad struct semantics, cc65/ca65 syntax and directives.

## v0.1 pitfalls (empirical, verified 2026-07-13; not in the manual)

- Loops CAN be recreated with inline-asm scaffolding: `asm("label:")` … C statements …
  `asm("    dec _var\n    bne label")`. Emission order is linear, so backward branches
  over lowered C code work. C-lowered bodies are long — loop-back edges often exceed
  the ±128 branch range and need `jmp` trampolines.
- Macros can NOT do this: function-like `#define` is never expanded; object-like
  `#define` expands in expression position only. Include guards work.
- `asm()` takes ONE string literal — adjacent-literal concatenation (`"a" "b"`) is not
  performed; fragments leak into the assembly as quoted garbage. Raw newlines INSIDE
  the literal are accepted (non-ISO, byte-identical to `\n` escapes — clangd will
  complain, the Web64 compiler won't), so multiline asm blocks are the readable way.
- `uint16_t` globals: initializers emit a proper `.WORD`, but all runtime operations
  (`=`, `++`) lower low-byte-only with no carry — do 16-bit manipulation in inline asm.
- Also accepted beyond the documented lists: `CIA1->pra`, `VICII->sprite0_x/…` register
  fields, struct member `++`/`--`, global-to-global assignment, and
  `.incbin label, "path"` in the assembler (labels the bytes like `.import binary`).

## Include resolution

Virtual includes only — `#include <stdint.h>`, `#include "include/game.h"`, etc.
Absolute host paths (`C:/...`, `/home/...`, `file:` URLs) are rejected. Bundled headers
resolve from `web64://c/include/...` before project-local lookup, except
`assets/generated.h`, which is regenerated from current project asset records each build.

## Bundled headers

| Header | Provides |
|---|---|
| `stdint.h` | `uint8_t`, `uint16_t`, `int8_t`, `int16_t` |
| `stddef.h` | `size_t`, `NULL` |
| `stdbool.h` | `bool`, `true`, `false` |
| `string.h` | tiny runtime `memset`, `memcpy` |
| `stdio.h` | tiny runtime `puts`, `putchar` (routed via stdout backend) |
| `stdlib.h` | tiny `abort` |
| `c64.h` | memory map constants, typed register structs, VIC-II/sprite/SID/CIA/keyboard/joyport/color/screen/KERNAL constants |
| `assets/generated.h` | generated C declarations for project assets + `_size` constants |

## c64.h highlights

- Memory/I/O: `C64_RAM_BASE`, `C64_BASIC_START`, `C64_SCREEN_RAM`, `C64_COLOR_RAM`,
  `C64_CHAR_ROM`, `C64_IO_BASE`, `C64_KERNAL_ROM`.
- Typed volatile pointer aliases: `VIC` / `VICII` (`c64_vicii_registers`), `SID`
  (`c64_sid_registers`), `C64_SCREEN`, `C64_COLOR`, `C64_CPU_PORT`.
- VIC-II: `VIC_BORDER`, `VIC_BACKGROUND`, `VIC_RASTER`, `VIC_CONTROL1/2`,
  `VIC_MEMORY_SETUP`, `VIC_SPRITE_ENABLE`, `VIC_SPRITE_X_MSB`, `VIC_SPRITE0_X/Y/COLOR`,
  `VIC_SPRITE_POINTER_BASE`, colors `VIC_COLOR_BLACK/WHITE/BLUE/LIGHT_GRAY/...`.
- SID: `SID_BASE`, `SID_VOICE1_FREQ_LO/HI`, `SID_VOICE1_PULSE_LO/HI`,
  `SID_VOICE1_CONTROL`, `SID_VOICE1_ATTACK_DECAY`, `SID_VOICE1_SUSTAIN_RELEASE`,
  `SID_VOLUME_FILTER_MODE`, waves `SID_WAVE_TRIANGLE/SAW/PULSE/NOISE`, `SID_GATE`.
- CIA/input: `CIA1_PRA/PRB/DDRA/DDRB`, `CIA2_PRA/PRB`, `KEYBOARD_COLUMN_PORT` etc.,
  `JOYPORT_1/2`, `JOY_UP/DOWN/LEFT/RIGHT/FIRE`, `JOY_RELEASED_MASK`.
- KERNAL: `KERNAL_CHROUT`, `KERNAL_CHRIN`, `KERNAL_GETIN`, `KERNAL_SCNKEY`,
  `KERNAL_PLOT`, `KERNAL_LOAD`, `KERNAL_SAVE`.

Joystick bits are active-low: test press with `!(joy & JOY_FIRE)`.

## Examples

```c
#include <c64.h>
#include <stdint.h>

uint8_t joy;

void main(void) {
    *(volatile uint8_t *)VIC_BORDER = VIC_COLOR_BLUE;
    VIC->background_color0 = VIC_COLOR_BLACK;   /* typed alias style */

    *(volatile uint8_t *)VIC_SPRITE_ENABLE = 1;
    *(volatile uint8_t *)VIC_SPRITE0_X = 80;
    *(volatile uint8_t *)VIC_SPRITE0_Y = 100;

    joy = *(volatile uint8_t *)JOYPORT_2;
    if (!(joy & JOY_FIRE)) {
        *(volatile uint8_t *)VIC_BACKGROUND = VIC_COLOR_RED;
    }
}
```

Inline assembly:

```c
void wait_raster(void) {
    asm("wait:\n    lda $d012\n    cmp #$80\n    bne wait");
}
```

## Generated asset header

For `assets/sprites/player.spr`, `assets/generated.h` contains declarations like:

```c
extern const unsigned char assets_sprites_player[];
#define assets_sprites_player_size 128
```

C shares the same asset names as assembly after lowering. Pair generated headers with
assembly/binary imports for data placement when the C subset can't express the copy loop.

## Mixed C and assembly

Use C for simple game logic and register setup; assembly for exact addressing modes,
cycle control, raster IRQs, music players, fast sprite movement, decompression, asset
copies. `asm_init_irq();` in C emits `jsr asm_init_irq`; define the label in assembly:

```asm
asm_init_irq:
    sei
    rts
```
