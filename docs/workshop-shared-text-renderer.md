# Shared native text renderer (Playtest phase 2)

The engine and Workshop now compile the same portable C implementation in
`src/text/text_window.c`. It has no SDL, Qt, file, clock or global-state
dependency. Engine adapters retain existing public functions and globals;
Workshop supplies independent fonts, text state and indexed pixels per session.

## Preserved behavior

- Native 16-byte word buffer, 18-column word-wrap threshold, font switching,
  suppressed leading spaces and duplicate newlines, and the long-word fix.
- Real text window at character coordinates (24,11)–(39,23): 16 columns,
  13 rows, 128×104 logical pixels. Full sessions default to cursor (0,12).
- Character hard wrapping, carriage returns, eight-pixel scrolling, clearing,
  underline, inversion, centering flags and temporary glyph restoration.
- Native PrintString wrapping/centering, cursor positioning bounds and backspace
  behavior, including the native erase/position order.
- Original glyph bit order and shared EGA palette. Engine glyph rendering keeps
  page selection/text offset; scrolling keeps its original page-zero behavior
  and tileset provenance copy/clear callbacks.
- Cursor glyphs can be painted without advancing. Polling, cadence and input
  editing remain with the native input routines until phase 3.

The engine's packed DOS record declarations are kept separate from the portable
callback structures. TextWindow retains its original eight-byte layout.

## Workshop presentation

`NativeTextWindow` uses private font copies and the shared routines.
`NativeTextViewport` displays a fixed framebuffer with nearest-neighbor scaling,
DOS 5:6 pixel aspect and centered letterboxing. Window resizing changes display
scale, never wrapping or logical dimensions. Missing rune fonts fail explicitly
when requested; normal-font replacement is no longer used.

The existing selected-response Game preview now renders into this window rather
than an unlimited, 18-column image. A selected response starts in a cleared
window at (0,0). This is deliberately distinct from a full conversation session,
which can start on the bottom row with prior text.

The response decoder still approximates dialogue execution and stops at certain
actions. Interactive Playtest still uses the prior Simulator. This phase
establishes shared rendering, not full Playtest accuracy. Phase 3 connects actual
dialogue execution, native input and waits; the later UI phase retires the
separate snapshot.

## Independent validation

`tests/native_*_reference.inc` preserve the pre-extraction functions from
`df25c19`; they are test-only and must not be refreshed from the new core.

- `text_window` is a portable C differential test, also built by standalone
  Workshop. It checks indexed pixels, cursor, buffer, fonts, effects and delays
  after operations from every starting row/column, plus glyph effects, clearing,
  cursor painting, backspace, native strings and scrolling in both directions.
- On Linux, `dialogue_renderer_parity` runs the nine phase 1 scripted conversations
  and an original TOWNE.TLK conversation through independent and shared
  renderers. Every artifact must match byte-for-byte, including event cursor
  positions/pixel hashes, snapshots, frame pixels, fonts, input and state.
- Workshop checks session isolation, missing fonts, native geometry and
  aspect-preserving resize without reflow.
- Existing engine and Workshop regression suites remain enabled.

Build/test through the normal root build, or without SDL:

```sh
cmake -S workshop -B workshop-build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build workshop-build --parallel 3
ctest --test-dir workshop-build --output-on-failure
```

The full-execution reference relies on GNU linker wrappers and is Linux-only.
The portable renderer differential test and Workshop viewport tests run on all
supported platforms.

