# macros

Defining and invoking assembler macros, parameter substitution, and macro-local labels.

## Defining and invoking

```asm
.macro set_border color
    lda #\color
    sta $d020
.endmacro

set_border $06
```

Multiple parameters are comma-separated:

```asm
.macro poke address, value
    lda #\value
    sta \address
.endmacro

poke $d020, $02
```

Accepted delimiter pairs: `.macro`/`.endmacro`, `!macro`/`!endmacro`, `macro`/`endmacro`,
`macro`/`endm`.

## Parameter substitution

Parameters can be referenced as `\name`, `{name}`, or bare `name`. The backslash form is
recommended — it avoids accidental replacement of normal identifiers.

## Macro-local labels

Use `%%name` for macro-local labels; each invocation gets a unique prefix during expansion:

```asm
.macro wait_raster line
%%loop:
    lda $d012
    cmp #\line
    bne %%loop
.endmacro
```

## Limits

Macro expansion depth is limited to prevent runaway recursion. Exceeding it produces a
macro-recursion diagnostic.
