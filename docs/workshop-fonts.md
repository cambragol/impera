# Editing fonts

Select a normal or runic font in the Resource Library's Fonts group. All 128 slots
remain available, including intentionally blank glyphs. Search labels or numbers;
slot numbers appear in tooltips. Left-drag paints pixels, right-drag erases. The
Erase tool, Copy, Paste and Clear operate on the selected glyph; strokes and edits
use Workshop's Undo/Redo. Paste requires matching dimensions.

Sample text is a project annotation, never a game text override. Preview this
font, its normal/runic companion, or the border glyphs used by signs and title
screen dialogs. Previews use the project's actual bytes, including edits. Missing
companion fonts are reported instead of substituting a checkout asset.

`IBM.CH` and `RUNES.CH`: 128 glyphs, each eight bytes (8×8 pixels). Rows are top to
bottom; bit 7 is the leftmost pixel. These fonts are used by Impera's color renderer.

`IBM.HCS` and `RUNES.HCS`: 128 glyphs, each 24 bytes (16×12 pixels). Rows are top to
bottom, two bytes per row; the first byte is the left half, most significant bit
first. This is confirmed by `ULTIMA_16ba_PrintChar` (24-byte glyph addressing),
`ULTIMA_17f4_ApplyCharEffects` (underline at bytes 22/23 and 24-byte inversion), and
the original `HER.DRV` entry 0x5d → 0x1282 (12 iterations of LODSW, writing both
bytes to consecutive screen addresses). It advances 16 horizontal and 12 vertical
screen pixels per character. No header, hidden glyphs or padding is discarded.
**Legacy Hercules font — not used by the SDL color renderer** remains visible
in the editor and package preview. Exporting a Hercules edit does not change
Impera's color rendering.

Only glyphs 123–127 are named as border corners/stroke: their usage is explicit in
`INTRO_08be` and the transfer-dialog border. Other non-text slots remain unnamed
until their usage is verified. Font previews show bitmaps, not text-control effects.
Narrative editors continue to decode original `PROPORT.PCS` for read-only previews;
that resource is not editable/exportable in this phase.
