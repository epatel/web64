# runtime-and-input

The embedded VICE-based C64 runtime: display, audio, storage, drives, keyboard, joystick,
and gamepad input.

The embedded runtime is shown inside the Code tab so you can edit source and inspect the
running program in one workspace. Options are grouped in collapsible sections:

- Runtime: start, pause, stop, reset, warp, live patch.
- Display: 1x, 2x, Fit, scanlines, fullscreen.
- Audio: audio enable, mute, SID quality, volume.
- Input: gamepad enable, joystick port, swap ports.
- Storage: quick save, quick load, snapshot import/export.
- Advanced: drive 9, runtime state, live patch state, load/start addresses, direct PRG
  memory load.

## Display

- 1x / 2x: fixed scale inside the visible viewport; may need scrolling or clipping if the
  panel is smaller than the rendered surface.
- Fit: fit the display to the available panel area — intended for normal responsive use.
- Scanlines: controlled through runtime video resources; disable in Display if unwanted.
- Fullscreen: applies to the emulator panel and focuses the canvas so keyboard input goes
  to the C64 without a second click.

## Audio

Audio must be enabled by user action (browsers require a gesture). Controls: audio on/off,
mute, volume, SID quality. SID quality options include FastSID and reSID modes.

## Drive 9

Drive 9 can be enabled/disabled in Advanced. Some C64 programs assume only drive 8 exists
and fail if drive 9 is present — disable drive 9 when testing those.

## Keyboard focus

The emulator canvas needs focus for C64 keyboard input. Fullscreen focuses it
automatically; otherwise click the canvas once if input doesn't work.

## Gamepad and joystick

Enable gamepad in the Input group. Options: gamepad enable, joystick port selector, swap
ports. Most C64 games use joystick port 2; some use port 1.

If a gamepad doesn't work: check gamepad is enabled, the correct port is selected, Swap
Ports isn't putting input on the wrong port, the emulator canvas has focus, and the
browser has detected the gamepad after a button press.

## Program behaves differently in the IDE than on the root emulator page

1. Confirm the same PRG bytes were loaded.
2. Confirm the same joystick port.
3. Disable drive 9.
4. Reset the runtime and reload.
5. Test with audio and warp disabled if timing is relevant.
