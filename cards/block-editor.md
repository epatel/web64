# block-editor

Creating and editing tile block sets (`.blk` / `.blocks`): character-backed blocks used
by the Map Editor.

A block set is a list of fixed-size blocks made from character indices, usually backed by
a charset selected from the project. It sits between a charset and a map: charset →
block set → map.

## Features

- New Block Set, Save Block Set, Export Asset, Export INC.
- Character-set dependency selection (block cells reference chars from that charset).
- Block add, duplicate, copy, paste.
- Tools: place character, erase, fill, copy, paste, duplicate, flip, shift, zoom.
- Block dimensions from 1x1 to 16x16 characters.
- Character palette sourced from the selected charset.
- 3x3 block tiling preview, byte preview, labels, offsets, generated include preview.

## Saving

Save Block Set commits draft edits back into the virtual filesystem and regenerates the
block include record. Block sets maintain draft state — save explicitly before saving the
project so the project contains current bytes.

## Workflow: build a tile map

1. Create a charset in Char Editor.
2. Create a block set in Block Editor and select the charset.
3. Create a map in Map Editor and select the block set.
4. Save both the block set and the map.
5. Import/include the generated `.blk`, `.map`, and `.inc` files from assembly.
