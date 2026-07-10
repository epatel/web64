# build-and-run

Building the PRG, loading it into emulator memory, starting it, and runtime controls.

## Main PRG actions

- Save PRG: compile and save/download the generated PRG.
- Load PRG: compile and write the PRG into emulator memory at the compile result load
  address. Does NOT start the program — use it when you want to inspect memory, set
  breakpoints, or start manually later.
- Start PRG: compile if needed, load if needed, then start the selected entry point by
  sending `SYS <entry-address>` as text input to BASIC (the runtime must be ready to
  receive keyboard/paste input). If source, assets, or project files changed since the
  last load, Start PRG reloads first.

The program is written directly into C64 memory through the runtime memory bridge when
available — faster than simulating disk loading.

## Runtime controls

| Control | Purpose |
|---|---|
| Start | Load if needed and run selected entry point |
| Pause | Pause runtime and refresh debug context |
| Stop | Stop current runtime program activity |
| Reset | Power reset C64 runtime |
| Warp | Toggle fast runtime execution (VICE warp mode) |
| Live Patch | Patch compiled changes into running memory |

## Workflow: new assembly program

1. Open `/ide`.
2. Write or paste assembly into the main source editor.
3. Set origin, commonly `$c000` for development PRGs.
4. Confirm diagnostics show Compile ready.
5. Click Start PRG.
6. Use the line map to inspect addresses and emitted bytes.
7. Save the project as `.web64proj`.

## If Start PRG lands on READY or a syntax error

Check that: the selected entry label points to executable code; program initialization
returns only when intended; the generated SYS address is correct; runtime keyboard/paste
input is available.
