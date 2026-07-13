# map-editor

Creating and editing maps (`.map` / `.w64map`): grids of block indices referencing a
block set.

A map stores block indices and references a block set, which in turn can reference a
charset (charset → block set → map).

## Features

- New Map, Save Map, Export Asset, Export INC.
- Block-set dependency selection.
- Map dimensions up to 1024x1024 cells; one-byte or two-byte index storage.
- Tools: pan, pencil, erase, picker, bucket fill, rectangle fill, ellipse fill, line,
  random brush, stamp capture, stamp paint.
- Fill map, fit-to-viewport, zoom-to-selection, zoom slider, grid toggle.
- Block palette, selected cell details, resolved block preview, mini-map overview,
  resize anchor controls, generated include preview.

## Saving and consuming

Save Map commits draft edits and regenerates the map include record. Use the exported
map bytes from assembly and interpret each cell as a block index in your engine.

See [block-editor](block-editor.md) for the full charset → block set → map workflow.
