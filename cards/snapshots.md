# snapshots

Runtime state tools: Quick Save/Load and VICE snapshot import/export.

Snapshots are emulator state, not source state — they do not replace `.web64proj`.

## Quick Save and Quick Load

Quick Save stores a temporary runtime state in browser memory; Quick Load restores it
during the current IDE session. Useful for short testing loops, but it is not a project
persistence mechanism.

## Export Snapshot

Saves a VICE snapshot file (usually `.vsf`) from the current runtime. Use when you want a
runtime state artifact outside the project file.

## Import Snapshot

Loads a `.vsf` or binary snapshot file into the runtime, when supported by the runtime
bridge.
