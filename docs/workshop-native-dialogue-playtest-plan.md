# Native dialogue Playtest implementation plan

## Goal and decisions

Replace Workshop's approximate conversation simulator with the dialogue execution
and text-window behavior used by the current Impera engine. The player-facing test
must reproduce the fonts, window geometry, wrapping, scrolling, prompts, input,
quotation handling, pauses, and dialogue-driven state changes.

Use current Impera behavior, including its intentional fixes, as the reference.
Matching an old DOS binary is a separate compatibility question. Do not fix an
engine quirk only in Workshop or silently make Playtest more forgiving.

Extract a portable shared C runtime used by both Impera and Workshop. Prefer
compiling the same functions over maintaining a second C++ interpretation of them.
Keep Qt as the presentation and host adapter. Preserve Workshop's standalone build
without requiring SDL, the engine executable, or a separate installed helper.

The plan's [phase 1 reference runner](workshop-dialogue-reference-runner.md) is
implemented. Production Playtest UI/runtime changes have not begun.

## Findings in the current implementation

| Component | Current behavior | Accuracy gap |
| --- | --- | --- |
| Playtest | `Dialogue::Simulator` in `workshop/dialogue.cpp` | Separate topic, question, action, and state implementation |
| Playtest display | Qt tree transcript and separate input field | No native text window, cursor, scrolling, or in-window input echo |
| Game preview | `previewResponse` in `workshop/dialogue_preview.cpp` | Selected-entry snapshot; branches stop and actions are approximated |
| Word layout | Port of two native word-buffer functions | Tested against extracted functions, but its character renderer assumes 18 columns and unlimited rows |
| Fonts | Snapshot reads IBM.CH and RUNES.CH | Live Playtest uses Qt text; project edits must reach the new raster renderer |
| Wait and Pause | Simulator trace records | Does not suspend the response and consume input at the actual action boundary |
| Recruitment and guards | Trace descriptions | Their actual effects and dependent responses are not exercised |

The native word-buffer threshold of 18 and the actual window width are different
things. Keep both behaviors; do not change the engine window to fit the existing
preview's assumption.

### Verified geometry

Gameplay text window 2 is initialized to character coordinates `(24,11)` through
`(39,23)`, inclusive. Its content is **16 columns by 13 rows**, or **128 by 104
logical pixels**, positioned at `(192,88)` in the original 320x200 framebuffer.
Initialization can place the cursor at `(0,12)`, the bottom row. Conversation
playback cannot simply assume it starts at `(0,0)` with an empty transcript.

`ULTIMA_16ba_PrintChar` advances the cursor, wraps at the real window edge, and
scrolls upward by eight logical pixels when it passes the bottom. The full native
scroll operation must be reproduced, not replaced with a growable image.

Monitor-aspect mode moves the 128-pixel sidebar while preserving its logical size.
Presentation stretches DOS pixels vertically by 1.2. Playtest should therefore keep
the same logical window dimensions at every zoom and apply the DOS pixel aspect
ratio only when displaying the framebuffer. Resizing the tools window must never
reflow the conversation.

## Architecture

```text
Project resources + chosen NPC + initial game state
                         |
                  isolated TestSession
                         |
          Shared native dialogue / text-window core
            |                  |                |
      raster updates      input/timer waits    state effects
            |                  |                |
         framebuffer      host event adapter   sandbox state
            |
      Qt pixel viewport

Same core + Impera host adapter -> normal gameplay
```

The shared runtime must own an explicit session context: dialogue buffers and
positions, pending operands, recursion/continuations, dictionary state, quote and
rune state, native text windows and cursor, font copies, input queue, and relevant
game state. Native widths, signedness, overflow/clamping behavior, and byte offsets
are part of the contract.

Host callbacks cover key polling/reading, elapsed ticks, frame presentation,
resource access, and effects that involve the wider world. Adapters must implement
the original behavior. Returning canned success for an unimplemented action is
not acceptable.

The engine's existing recursive dialogue flow can remain blocking initially.
Workshop runs it on a dedicated worker thread, with interruptible input/timer
callbacks. Qt receives immutable framebuffer snapshots and typed state/trace
events. No Qt widgets are accessed from the worker. Cancellation must wake waits,
unwind safely, and join the worker; do not terminate a thread or use an unsafe jump
across C++ frames. Make all state session-owned so two editors cannot interfere.

If converting the recursive control flow to resumable execution becomes necessary,
perform that as a shared-core refactor and prove parity against the original
functions first. Do not introduce a separate Workshop state machine as a shortcut.

## Native behavior to share

| Source | Responsibility |
| --- | --- |
| `src/talk.c` | Entry dispatch, automatic text/quotes, dictionary expansion, word buffering, operand consumption, keywords/aliases, questions, fallbacks, introduction checks, payment/inventory/karma actions, and conversation exits |
| `src/1000.c` | Character output, native text windows, font selection/effects, cursor positioning, foreground/background colors, clearing, and backspacing |
| `src/graphics/grap_buf.c` | Glyph rasterization and framebuffer scrolling, with a small portable framebuffer interface |
| `src/3000.c` | String input, character limits, echo, Backspace/Left, Escape, and Enter behavior |
| `src/2000.c` | Key normalization and timed waits; adapt clocks and input without changing their semantics |
| `src/6000.c` and `src/vars.c` | Keyword matching, authoritative dictionary/text/constants, and frame artwork behavior |
| NPC, town, inventory, and party routines | Actual action outcomes using isolated copies of the selected context |

Reuse validated project resource access and the original resource loaders where
appropriate. Load edited IBM.CH, RUNES.CH, TLK, and any relevant overlay/rule resources
from the Project; fall back to that project's base resources. Never substitute a
Qt/system font or silently replace a missing rune font.

Native character effects temporarily modify glyph data before restoring it. Use
session-local font buffers rather than mutating Project resources. Preserve glyph
bit order, palette indices, underline/inversion, and rune charset behavior.

## User interface

Use one **Playtest** action opening a dedicated window:

```text
[Restart] [Setup]                                  [Close]

             native conversation frame
             +--------------------------+
             | 16 x 13 native characters |
             | scrolling game text      |
             |                          |
             | Your interest?           |
             | : native input/cursor    |
             +--------------------------+

[Zoom]                       [optional Actions panel]
```

The diagram is illustrative; use the engine's actual frame pixels and dimensions,
not a newly drawn decorative border. Keep the glyph area exactly 128x104 logical
pixels and validate the surrounding frame crop against the engine framebuffer.

- Capture keyboard input in the pixel viewport. Native input owns echo, accepted
  keys, length limits, and editing. Remove the ordinary Qt Send/input row from the
  player-facing experience.
- Wait for key must remain blocked until an accepted key arrives. Pause uses the
  native 28-tick loop (currently 55 ms per tick), including early key interruption
  and clearing the keyboard buffer. A wait key cannot also become the next answer.
- Apply the native cursor behavior and timing. Restart rebuilds an initial session;
  closing the window cancels it promptly even during a wait.
- Keep Setup collapsed by default. Offer understandable choices for party, supplies,
  introduction history, and the selected NPC/location. Default to the current
  Project's starting state and real NPC identity where available. Show seeded test
  variants for first-encounter randomness rather than forcing a different greeting
  path inside the interpreter.
- Keep action traces and state details in an optional adjacent panel. Clicking a
  trace navigates to the authoring entry/byte; it must not consume a game key.
- Preserve the current snapshot as a temporary migration tool only. Once native
  playback supports normal testing, remove the redundant Game preview action and
  static DOS preview tab. Diagnostics stay available without another player preview.
- Keep authoring text readable in the normal Workshop font. Original fonts belong
  in Playtest, not the editing fields.

## Implementation phases

### Phase 1: Establish a native reference runner

Build a headless test harness around the actual Impera dialogue and text-output
functions. Supply deterministic keys, time, RNG seeds, edited TLK/font resources,
and a known native game/NPC state. Exercise full conversation entry, not just a
single response. Capture framebuffer pixels, window/cursor state, font selection,
input requests, action effects, and entry-relative byte offsets.

Record native starting-window state, frame crop, input cursor cadence, clock/frame
updates during waits, and the effects of engine build flags. The existing matcher
and word-buffer oracles are useful pieces, but their simplified PrintChar wrapper
does not validate native clipping or scrolling. Retain an independent native
reference after extraction so testing does not merely compare the new core to itself.

**Exit:** scripted conversations run through the real code and produce reproducible
reference outputs, including at least one overflowing text window and interrupted
Pause/Wait sequence.

### Phase 2: Extract the shared text-window renderer

Factor the native text window, word buffer, font/effect operations, palette, glyph
drawing, scroll, clear, backspace, and cursor logic into a portable core. Replace
engine calls with thin compatibility wrappers. Use the same code in a minimal Qt
pixel viewport and keep fonts/session buffers isolated.

Preserve the 18-column word-buffer rule alongside the 16-column character-window
wrap rule. Show fixed dimensions with aspect-correct nearest-neighbor scaling.

**Exit:** pixels and cursor state equal Phase 1 at each render boundary, including
wrapping, repeated quotes, mixed speech/narration, runes, and scrolling. Ordinary
Impera rendering regressions also pass.

### Phase 3: Extract conversation execution and connect input

Move the actual dispatch/interpreter/keyword/question routines into the shared
runtime. Preserve native entry framing, nested transfers, alias precedence,
operand handling, names, and all blocking input paths. Integrate the worker/input
adapter, native string editor, timed Pause, Wait, and restart/cancel lifecycle.

Port behavior from current engine source without cleaning up quirks during the
extraction. Keep source-offset events separate from player text.

**Exit:** scripted topic/question/name conversations match the reference in text,
pixels, branch decisions, input consumption, and virtual timing. Qt remains
responsive while the session waits.

### Phase 4: Supply the complete isolated NPC/game context

Expand the current Sandbox beyond gold, karma, party size, and a boolean introduction.
Load the actual NPC identity, conversation linkage, party/recruit records, per-NPC
introduction history, equipment/supplies/quest state, and relevant location/rules.

Use the actual inventory, payment, recruitment, and guard routines with sandbox
state and faithful host adapters. Handle insufficient funds, full parties,
non-recruitable NPCs, item ownership/caps, and action-driven early exits. Preserve
clock/RNG effects of native update calls where they affect dialogue outcomes.

World rendering can be omitted from a conversation-only viewport, but world effects
cannot be replaced with success traces. Any action still lacking its required
context is reported outside the viewport as unsupported and blocks an accuracy
claim for that session. Normal full-game Test Mod remains available for verifying
movement/combat and consequences outside the conversation.

**Exit:** every action exposed in the NPC dialogue authoring UI has verified native
success/failure behavior and sandbox mutations. No project, installed mod, game
resource, or real save is written by the session.

### Phase 5: Replace the current Playtest UI

Connect the native viewport to ConversationEditor, use Project fonts/resources,
keep Setup and traces optional, and remove the player-facing Qt transcript/Send
controls. Preserve source navigation, validation, and draft handling.

Edits during a session require Restart to take effect; do not hot-swap pointers or
silently restart mid-input. Reset/close must release pending events and state.
Once this is ready, retire the separate static Game preview presentation.

**Exit:** the window stays the correct size at different zooms and DPI settings,
keyboard focus works reliably, and the same modded conversation is observed in
Workshop and in Impera.

### Phase 6: Prove parity and ship across platforms

Run a differential corpus through the independent reference and shared runtime.
Use virtual time for repeatable tests and perform manual real-time verification.

Cover all original conversations where original assets are available, plus small
synthetic fixtures that exercise every opcode and boundary. Walk different answers
and setups; testing only one transcript per NPC is insufficient. Include:

- 15-character topic input, name-input limits, empty answers, Backspace/Left/Escape,
  mixed case, aliases, first-match behavior, and built-in commands;
- all quotation paths, speech/narration endings, long words, dictionary boundaries,
  every window row/column edge, clearing, scrolling, cursor, and rune transitions;
- two-newline and exceptional Pause/Wait forms, key interruption/consumption, and
  input queued immediately before/after a timed event;
- nested question transfers, fallback ordering, introduction/karma conditions,
  explicit exits, and loops that must remain cancellable;
- payment/inventory/party/guard effects for every reachable success/failure path;
- modified fonts and dialogues, save/load/import round trips, and missing/corrupt
  resources failing clearly without fallback visuals;
- two isolated sessions, repeated Restart, closing during each wait mode, stale
  trace invalidation, and no persistent state leakage.

Required CI covers Linux, Windows, macOS Intel, and macOS ARM, including standalone
Workshop builds and packaged launches. Platform-independent pixel comparison uses
logical framebuffer pixels; real-time tests allow scheduler jitter without altering
the interpreter's requested ticks or key-consumption order. RNG comparisons use
matched seeds and the appropriate platform's native reference.

**Exit:** exact logical-pixel, branch, input, and state parity for the supported
dialogue scope; timing requests match; all four release packages pass smoke tests.

## Recommended starting point

Implement Phase 1 first. It establishes the real geometry and a full-execution
reference before moving production code. Phase 2 can then deliver the correct
font/window/scroll presentation while the old simulator remains temporarily in
place. Do not call that interim result fully accurate until execution, input, and
action-context phases are complete.
