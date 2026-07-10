# live-patching

How Live Patch writes compiled code and asset changes into the running emulator, and when
to reload instead.

Live patching writes compiled changes into the running emulator so effects are visible
without restarting the program.

## How it works when enabled

1. The IDE notices source or asset changes.
2. The compiler rebuilds after the edit/paste debounce.
3. If diagnostics are clean, the IDE determines whether the change can be patched.
4. The runtime pauses briefly.
5. The relevant memory bytes are written.
6. The runtime resumes unless it was manually paused.

## Asset live patch (shadow-buffer workflow)

Character and sprite edits: the editor updates local JavaScript bytes immediately; the
grid and preview update from local state; a debounced project commit updates the virtual
binary file; if the asset is imported into the running program and the line map contains
a memory range for it, the dirty bytes are patched into emulator memory. Much faster than
writing every brush stroke directly to C64 memory.

## Full PRG live patch

If an edit changes code, or the changed asset cannot be isolated, the IDE can patch the
full compiled PRG body into memory.

## When live patch waits (does not write)

- Compilation has diagnostics.
- The runtime is not started.
- No compatible memory write bridge is available.
- The changed asset is not imported into the currently loaded program.
- The current program code key no longer matches the loaded program.

Status messages indicate whether bytes were patched, compilation is waiting, or an asset
is not imported into the running program.

## When to reload the PRG instead

- You changed startup code or initialization state.
- Your program copies assets to another address at runtime.
- Your program decompresses or transforms data after load.
- You changed a memory layout assumption.
- You want to reset C64 state completely.

Live patch writes memory bytes; it does not rerun your initialization logic.

## Patch reported but display doesn't update

Likely causes: the program copied the asset elsewhere at startup; the asset was
transformed/decompressed by your code; the asset isn't imported into the loaded PRG; the
program reads from a different address than the import range; the change affects
initialization code that needs to run again. Reload and restart the PRG when in doubt.
