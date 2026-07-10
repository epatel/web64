# debugger

Source-oriented debugging: breakpoints, pausing, registers, memory dumps, and the line map.

The Debugger tab wraps the embedded runtime with: manual breakpoint entry by address or
symbol, line breakpoints from the line map, pause/resume, debugger state refresh, register
display, memory dump by address and length, and trace/current-line highlighting in the
debugger and source/line map views.

## Breakpoints

Three ways to add one:

1. Enter an address or symbol in the Debugger tab and click Add Breakpoint.
2. Click the breakpoint marker in the Inspector line map.
3. Click a line breakpoint entry in the Debugger line list.

Addresses are normalized to 16-bit C64 addresses. When a breakpoint hits, the runtime
pauses, the IDE switches to debugger context, captures register and memory state (when the
runtime bridge supports it), and highlights the corresponding source and trace line.

If a breakpoint does not hit, check: the address exists in the current compile line map;
the loaded program matches the current compile result; the runtime monitor breakpoint
bridge is available; the code path actually executes.

## Pausing

Manual pause acts like a debugger pause: it captures the current execution address when
possible and highlights the active line if the address maps to compiled source.

## Registers

The register panel shows PC, A, X, Y, SP, and Flags. Available fields depend on runtime
monitor/debug exports; unexposed values show blank or placeholder state.

## Memory

The memory panel reads a short range of C64 memory: enter a start address or symbol
(e.g. `$0400`, `$d020`, `screen_buffer`) and a byte count from 1 to 256, then refresh.

## Line map

One of the most useful debugging tools. Shows per source line: line number, mnemonic and
operand, start address, emitted bytes, branch target and relative offset, breakpoint
state, and trace state. Use it to validate branch offsets, asset import ranges, compiled
code size, and generated addresses.
