# sid-editor

The SID Editor: inspecting, previewing, and generating includes for imported or generated
`.sid` binaries.

Use SID Editor when you already have a PSID/RSID-style binary. Use SID Tracker
([sid-tracker](sid-tracker.md)) when you want to edit a Web64-native `.w64sid` tracker
source file — the two are separate editors.

## Features

- Imported/generated SID asset selection from the virtual filesystem.
- Metadata display: title, author, release string, load/init/play addresses, song count,
  start song, model, and speed fields where available.
- Text metadata editing for project records.
- SID preview in the Web64 runtime, and stop preview.
- Include generation for imported or generated SID assets.

Adding a `.sid` file to the project (or creating one with New) creates/imports a SID
binary record for the SID Editor.
