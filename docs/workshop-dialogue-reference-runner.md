# Native dialogue reference runner (Playtest phase 1)

Phase 1 is implemented as `impera_dialogue_reference`, a headless test/developer
executable built with the engine's native sources. Since phase 3, Workshop
Playtest uses shared native conversation execution and input.

After phase 2, the reference keeps test-only copies of the original character,
string, word-buffer, backspace and raster/scroll functions from `df25c19`.
`impera_dialogue_shared_reference` runs the same conversations through the
extracted production renderer. The `dialogue_renderer_parity` test compares their
complete artifacts, including per-render events and framebuffer snapshots.
The fixtures' hashes are included in reference provenance.

Phase 3 also preserves independent dialogue, matching, input and dictionary/
keyword-table fixtures. The corpus now adds name recognition, insufficient
funds, nested questions and special-key Left editing. Workshop's own standalone
host is checked against the same artifacts by `workshop_dialogue_host_parity`.

## What executes

The runner stages a conversation in the native scratch buffer and enters the
same post-load conversation path as `TALK_127e_StartTalk`: description/first meeting
or greeting, the topic input loop, questions/answers/fallbacks, and farewell.
These are actual `talk.c` functions, not a port or the Workshop Simulator.

The actual character/string routines in `1000.c`, native keyboard input editor in
`3000.c`, key normalization/tick waits in `2000.c`, keyword matcher in `6000.c`,
font glyph operations, frame drawing, and `GRAP_BUF` raster/scroll routines execute.
Graphics calls are routed directly to the native buffer renderer without creating
an SDL window. The runner still links the engine's normal SDL build dependencies.

The generated build-only copy of `talk.c` adds an observer before each dispatched
byte and a bounds check in the native string scanner. It otherwise contains the
entire original source. Renderer, input, sleep, and keyboard-clear observers use
linker wrappers; they do not replace dialogue decisions or text layout. Native
frame updates execute with map rendering disabled (`D_58a4 = 0`).

Scripted key polling, a virtual clock, and a fixed seed provide repeatability.
The native routines determine the accepted keys, input editing/limits, uppercase
conversion, cursor animation, wait-key consumption, and requested delays.

## Build and run

For the current Linux engine build:

```sh
cmake -S . -B build -DU5D_BUILD_TESTS=ON
cmake --build build --target impera_dialogue_reference --parallel 3
ctest --test-dir build -R '^dialogue_reference' --output-on-failure
```

The CTest cases create artifacts below `build/dialogue-reference-runtime` and
`build/dialogue-reference-tlk-runtime`. Original IBM.CH and RUNES.CH are required;
there is no replacement font. `U5_GAME_DIR` overrides the default source game
folder, or use `--game` explicitly.

Run a specific fixture:

```sh
build/impera_dialogue_reference --case pause_interrupt \
  --output build/dialogue-reference-runtime --game "Ultima 5"
```

Run an edited or original TLK resource:

```sh
build/impera_dialogue_reference \
  --tlk "Ultima 5/TOWNE.TLK" --npc 1 \
  --script tests/dialogue_reference_bye.tsv \
  --output build/my-dialogue-reference --game "Ultima 5"
```

`--npc` selects the TLK conversation identity. Phase 1's sandbox NPC slot is 5 in
map 13; this is not yet a loader for the selected NPC's actual schedule/world state.
The TLK header, offsets, buffer limit, operand boundaries, five initial entries,
and structural tail are validated before native execution. No game files or saves
are written. Execution/input budgets fail clearly if a script cannot finish or an
invalid branch scans outside the native 1024-byte dialogue buffer.

Optional parameters:

- `--font FILE` and `--runes FILE`: use edited 1024-byte font data.
- `--seed N`: fixed native RNG seed, from 0 through 65535.
- `--x N` and `--y N`: initial window-relative cursor position; default `(0,12)`.
  Use with one selected fixture or a custom TLK.
- `--introduced 0|1`: introduction history for custom TLK sessions.
- `--help`: command syntax.

Key scripts contain one `delay_ms native_key_code` pair per line. Each delay
starts on the first native poll for that key, not at an absolute wall-clock time.
Examples: 13 is Enter, 8 is Backspace, 257 is special-key Left, 27 is Escape,
32 is Space, and 98/121/101 type `bye`. Codes 1–511 are accepted; bit 256
sets the native extended-key flag. Lines beginning with `#` are comments. The native input
editor performs character echo and editing; the script does not submit strings
directly into the interpreter.

## Reference artifacts

Each fixture/custom run writes:

| File | Contents |
| --- | --- |
| `manifest.json` | Source fingerprint, font CRCs, seed/context, geometry, cursor/font state, virtual elapsed time, trace/pixel hashes, effects, and coverage limits |
| `events.jsonl` | Ordered opcode/operand, glyph, input, frame-update, sleep, scroll, and clear events; native cursor/font/state and cropped framebuffer hash at each event |
| `snapshots.bin` | Full 128x104 indexed pixels at input, cursor/edit, scroll, clear, start, and finish boundaries |
| `initial-frame.bin` | Native 320x200 indexed framebuffer before conversation entry, including the actual frame |
| `frame.bin` | Final native 320x200 indexed framebuffer |
| `window.bin` | Final 128x104 indexed conversation content |
| `conversation.tlk-entry` | The staged native conversation bytes |
| `keys.tsv` | Exact replay script |
| `normal.font`, `runes.font` | Exact font inputs used for the run |

Snapshot records are eight header bytes followed by 13,312 indexed pixels: a
little-endian 32-bit event sequence number, a little-endian 32-bit virtual time in
milliseconds, then row-major pixels. The sequence number matches `events.jsonl`.
Native glyph events are observed before the character renderer advances its
cursor; later input/scroll/final events capture the resulting cursor state.
Opcode offsets identify actual bytes inside entries; glyph events retain their
execution context, including engine-generated text that has no source byte of its
own. `a`/`b` are event-specific: glyph character coordinates, scroll origin,
absolute opcode offset/operand indication, or key index.

The framebuffer is indexed EGA data with values 0..15, not RGB and not aspect
corrected. Its logical text rectangle is `(192,88,128,104)`, or 16x13 glyphs.
Presentation uses a horizontal:vertical pixel ratio of 5:6. A viewer must apply
that ratio without changing logical window dimensions or reflowing text.

Artifacts are local generated data and must not be committed as original-game
artwork. Preserve a reference directory before refactoring the core in phase 2.
The source fingerprint covers linked native source, native headers, and the
reference runner; CMake tracks them so a source change updates the fingerprint.

## Automated coverage

Every scenario executes twice and compares its complete trace hash, final pixels,
cursor/font, elapsed virtual time, input consumption, and recorded effects.
Additional assertions verify exact eight-pixel scrolling, native glyph bit order
and colors, restored font buffers, and scenario-specific decisions/effects.

| Fixture | Verified behavior |
| --- | --- |
| `questions` | Native alias matching, question transfer, answer selection, karma increase, and key inventory increment |
| `fallback` | Unknown answer follows the actual fallback without answer effects |
| `scroll` | Twenty-two description rows overflow the real 13-row window; each scroll moves pixels exactly and clears its bottom row |
| `pause_interrupt` | Pause consumes its interrupt after two native ticks; a distinct delayed key satisfies Wait; subsequent topic input remains intact |
| `pause_full` | Pause requests all 28 native 55-ms ticks (1,540 ms) before later topic input |
| `editing` | Native Backspace and Escape editing produce `JOB` and `WORK` |
| `input_limit` | Topic input accepts exactly fifteen characters, then handles the next reply independently |
| `introduction` | Actual seeded first-meeting selection reproduces the same execution twice |
| `runes` | Six glyphs use the original runic font, then normal charset is restored |
| `ask_name` | Party-name recognition sets the NPC introduction flag and follows the introduced greeting |
| `payment` | Insufficient funds returns to native topic input without deducting gold |
| `nested` | A karma condition transfers into a second question and executes the selected answer |
| `left_editing` | The native extended-key flag lets Left edit the input as in the engine backend |
| `dialogue_reference_tlk` | A real original TOWNE.TLK conversation is loaded by identity and completes with scripted native input |

## Boundaries and findings

- This initial harness uses GNU linker wrappers and is enabled on Linux. Windows
  and macOS keep their existing builds/tests. Cross-platform reference/host
  integration remains part of the later shared-core work and phase 6.
- This is the post-resource-load native conversation path. Native disk selection,
  map-facing Talk command prechecks, special engine-controlled NPC/shop paths,
  and a complete moving world/save context remain outside its scope. Phase 4 adds
  recruitment, guards, beggar payments and all valid inventory operands, with full
  initial/final private game-state artifacts. See [context validation](workshop-native-dialogue-context.md).
  Do not describe a custom run as full-world parity.
- Rendering uses an EGA gameplay profile, real fonts, the native frame, and the
  bottom-row initial cursor. Other video-driver modes and real-time scheduler
  jitter are outside phase 1's reference.
- The normal context is Avatar/Iolo/Shamino, gold 100, karma 50, zero keys, and
  introduced NPC slot 5. Context fields are recorded so later comparisons use
  the same inputs. The seeded first-meeting fixture differs intentionally.
- `ULTIMA_1b24_ClearKbdBufferImpl` is currently a native stub. The runner records
  clear requests and executes that stub without inventing queue clearing. Its
  interrupting Pause key is consumed by native polling; future script keys are
  not silently discarded. Any subsequent fix must change Impera and update the
  independent reference together.
- The native word-buffer threshold is 18, while the character window wraps at 16.
  Actual execution preserves both. Unlike the existing Workshop snapshot, the
  reference never grows the text area or substitutes eighteen visible columns.

Next: phase 2 can extract the shared text-window renderer and compare its frames,
cursor, fonts, wrapping, and scrolling against these native outputs. Playtest
remains an approximation until the later execution/input/context phases are done.
