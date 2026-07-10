# Breakout

Single-file Breakout game for the C64 (web64 IDE).

## Files
- `breakout.asm` — complete game, one file (~620 lines)

## Key facts
- Entry label `start`; contiguous single-segment layout starting at `$0840`
- Sprite data first at `$0840` (64-byte aligned, must stay below `$4000`), code after
- Controls: joystick port 2 (left/right, fire) or keyboard A/D/SPACE, read via
  KERNAL current-key at `$cb` (A=10, D=18, SPACE=60, none=64)
- Ball launches automatically after a short delay; SPACE restarts after
  game over / win

## Testing in the IDE
Keyboard input cannot be driven by browser automation (synthetic keys never
reach the emulated C64) — verify by watching autonomous behavior (ball
physics, brick collisions, wall bounces), not by playing.
