# troubleshooting

Symptom-keyed checklists for common Web64 IDE problems.

## Pasting a large source block makes the IDE slow

Large pastes are debounced before background compile — wait briefly. If diagnostics still
lag, check for very large generated sources or recursive macros.

## The browser page goes black

Usually an uncaught JavaScript exception. Current builds guard compiler failures and
should show diagnostics instead. If it happens: reload the IDE; reopen the last saved
`.web64proj`; paste the code into a small temporary source file first; check for
unterminated strings, unexpected macro recursion, or very large generated output.

## Include file not found

Check: the file exists in the virtual file tree; the path uses forward slashes; the path
is relative to the current source file or is an unambiguous project path; the extension is
supported as text or binary.

## Binary data appears as garbage

Check: the asset is included at the address your runtime code expects; sprite data uses
64-byte slots in Web64 IDE `.spr` exports; your copy routine copies 64 bytes per sprite
slot (or intentionally skips the padding byte); your VIC-II memory bank and sprite
pointers match the loaded address; you are not copying from the PRG load address instead
of the label exported by `.import binary`.

## Live patch says asset changed but display does not update

The running program may have copied the asset elsewhere at startup, transformed or
decompressed it, or reads a different address than the import range; the asset may not be
imported into the loaded PRG; the change may affect initialization code that needs to run
again. Reload and restart the PRG when in doubt.

## Start PRG goes to READY or syntax error

Check: the selected entry label points to executable code; initialization returns only
when intended; the generated SYS address is correct; runtime keyboard/paste input is
available.

## Breakpoint does not hit

Check: the address exists in the current compile line map; the loaded program matches the
current compile result; the runtime monitor breakpoint bridge is available; the code path
actually executes.

## Gamepad does not work

Check: gamepad enabled in Input; correct joystick port selected; Swap Ports isn't putting
input on the wrong port; emulator canvas has focus; the browser has detected the gamepad
after a button press.

## Drive 9 causes problems

Disable Drive 9 in Advanced. Some programs expect only drive 8 and fail when another
drive is attached.
