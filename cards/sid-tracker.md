# sid-tracker

The SID Tracker: editing `.w64sid` tracker source files and generating `.sid`/`.bin`/
`.inc` outputs with the fixed Web64 SID driver.

`.w64sid` is the Web64 source-truth JSON format for songs built with the fixed Web64 SID
driver (`web64-fixed-v1`). For already-existing PSID/RSID binaries, use the SID Editor
instead ([sid-editor](sid-editor.md)).

## Features

- New W64SID and Save W64SID; explicit W64SID runtime preview and stop preview.
- On save, when validation/export succeeds, derived records are regenerated next to the
  source: `.sid`, `.bin`, and `.inc`.
- Deterministic validation against the `web64-fixed-v1` driver descriptor; driver limit
  display for patterns, rows, instruments, table rows, and subtunes.
- Order editor (add/remove order steps); subtune start, loop, and end controls.
- Pattern editor for three SID channels: note, instrument, effect, parameter fields.
- Input: piano-style keyboard note entry, arrow/Home/End/Tab/Enter navigation,
  Delete/Backspace clearing, Ctrl/Cmd-Z/Y undo/redo.
- Pattern clone, row insert/delete, grouped undo/redo, channel mute/solo, follow
  playback, pattern loop, octave selection, speed and tempo controls.
- Metadata: title, author, released, load address, music data address.
- Instruments: waveform, ADSR, pulse width, default volume, duplicate, audition.
- Tables: wave, pulse, filter, arpeggio, and vibrato command/value/loop rows.

## Driver limits

The fixed driver supports three voices, PSID export, rests/cuts, supported waveforms,
selected table commands, and capability-checked effects. Unsupported effects, RSID
export, multi-SID settings, or invalid limits BLOCK generated output with diagnostics —
save succeeds for the source but derivatives are only regenerated when validation is
clean.

For `.w64sid` include output, a manual include at the generated path is preserved as an
override.

## Workflow: create a Web64 SID song

1. Open SID Tracker and create a `.w64sid` file.
2. Edit metadata, instruments, pattern rows, order, and subtune loop points.
3. Use Preview W64SID for runtime playback.
4. Save W64SID to regenerate `.sid`, `.bin`, and `.inc` outputs when validation is clean.
5. Include the generated `.inc` or import the generated payload from assembly.
