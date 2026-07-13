# assembler

The browser-local 6502 assembler: labels, numbers, expressions, directives, instructions,
origin, and compilation behavior.

Compilation is browser-local; no native assembler is started. The compile result feeds
diagnostics, the line map, entry label selection, and debugging metadata. The editor has
a syntax-highlighted background layer over a text area; background compilation is
debounced, so a large paste is treated as one completed edit. Assembly and Web64 C source
both compile through the same browser-local build graph — generated C assembly is handed
to this assembler pipeline (see [c-compiler](c-compiler.md)).

## Labels and symbols

```asm
start:
    lda #$00
    sta $d020
    rts

BORDER_COLOR = $d020
SCREEN_RAM = $0400
RASTER .equ $d012      ; .equ, !equ, and equ also accepted
```

## Numbers and expressions

```asm
$c000       ; hexadecimal
0xc000      ; hexadecimal
49152       ; decimal
%10101010   ; binary
'A'         ; one-character literal

<label      ; low byte
>label      ; high byte
value + 1
table + index * 2
(screen + 40),y
```

## Data directives

```asm
.byte $01, $02, $03    ; also !byte, byte, db
.word start, $c000     ; also !word, word, dw
.text "HELLO"          ; also !text, .ascii
.fill 40, $20
```

## Origin

The project origin sets the default load address if the source doesn't set one; the
default project source begins at `$c000`. In source, any of:

```asm
* = $c000
.org $c000
!org $c000
org $c000
```

## Instructions

The assembler supports the official 6502 instruction set plus a broad set of undocumented
opcodes used in C64 coding. Zero-page vs absolute mode is resolved from expression values
when possible.

Branches are emitted with signed relative offsets. The line map shows the target address
and relative byte value — useful for debugging loops and range errors. A branch target
outside -128 to +127 bytes produces a branch-out-of-range diagnostic.

```asm
wait:
    lda $d012
    cmp #$30
    bne wait
```

## Compiler output (Inspector)

- Diagnostics: errors and warnings with source line navigation.
- Line map: source line, start address, emitted bytes, targets, branch offsets, and
  breakpoint toggles.
- Symbol count and output byte count in summary buttons.

## Entry point

The IDE looks for likely entry labels: `start`, `entry`, `_start`, `main`, `mainloop`,
`init`. You can also select an entry label manually. Start PRG sends a BASIC
`SYS <entry-address>` to the embedded emulator.
