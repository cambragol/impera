# Native dialogue execution and input (Playtest phase 3)

Impera and Workshop now compile the same C conversation interpreter, keyword
matcher and input editor. Interactive Workshop Playtest no longer uses
`Dialogue::Simulator`; its remaining model tests do not drive the player UI.

## Shared execution

`src/text/dialogue_runtime.c` contains the extracted native dispatch, entry
framing, dictionary expansion, quote/rune handling, aliases, built-in commands,
questions, fallback selection, nested transfers, name recognition and introduction
checks. Native word buffering and rendering remain in the phase 2 shared core.

An explicit context references dialogue buffers, cursor/operand state, input,
word state and host operations. Workshop binds these to private storage for each
execution. Impera's compatibility adapter binds its existing variables and actual
game operations. Dictionary and built-in keyword initializers are shared with
the engine; its runtime table pointers and optional translated text lookup are
preserved.

`src/text/text_input.c` shares the actual native cursor painting, key
normalization, string echo, character limit, Backspace, special-key Left, Escape
and Enter behavior. Host input retains the extended-key flag used by the native
keyboard backend.

Wait blocks for native input. Pause performs the actual 28 iterations of frame
update, key poll and one 55 ms tick, ending early when a key arrives. A consumed
Wait/Pause key cannot also become an answer. ClearKbdBuffer remains the current
engine's no-op; queued subsequent keys are not silently discarded.

Reads are bounded by the conversation buffer. Cooperative cancellation checks
allow recursive execution to unwind without terminating threads or jumping
across C++ frames. Excessive nesting fails explicitly.

## Workshop worker and UI

A worker runs the shared runtime with private conversation/font copies, text
window, input state and sandbox state. It sends immutable images and source-linked
action events through queued Qt connections. Widgets are accessed only on the UI
thread.

The native viewport captures keyboard input and shows in-window echo and cursor
animation. It replaces the ordinary Qt reply field and Send button. The optional
actions pane remains on the right. Setup is collapsed; its seed controls the
platform-native first-meeting random draw instead of forcing an announcement.
Restart cancels and joins the old worker before rebuilding a fresh execution.

Closing the tools window, leaving the editor or destroying it wakes pending
sleeps and joins the worker. Generation guards reject stale images/actions from
a replaced execution. Editing does not hot-swap the running conversation; Restart
uses the latest bytes and fonts, and stale action links are disabled.

## Scope and remaining work

Phase 4 adds [isolated NPC/game context](workshop-native-dialogue-context.md),
including shared native payment, inventory, recruitment and guard effects.
No project, mod package, game files or real save are written by Playtest.

Phase 5 supplies the [native frame crop and zoom controls](workshop-native-dialogue-presentation.md)
in a dedicated Playtest window. The selected-response snapshot has been removed
from the UI; its decoder remains only as a test oracle.

## Validation

- The independent reference preserves pre-extraction native dialogue, text,
  input, matching and constant tables. It does not call the new interpreter.
- Full engine-adapter parity compares complete artifacts byte-for-byte.
- Workshop host parity replays the expanded native corpus with virtual time.
  Each glyph/scroll boundary must match indexed pixels, cursor, font, operands,
  source offsets and time. Branch/opcode order, final state and input consumption
  must also match.
- Cases cover aliases/questions/fallbacks, overflowing windows, full/interrupted
  Pause, Wait, Backspace/Escape/Left, input limits, first meetings, runes,
  player-name recognition, insufficient funds and nested karma questions, plus
  an original TOWNE.TLK conversation.
- UI tests cover source-linked actions, keyboard capture, no Project mutation,
  simultaneous sessions, repeatable restart, UI heartbeats during waits, and
  prompt cancellation during Wait and Pause.
- Standalone Workshop remains Qt-only, without SDL or an external engine/helper.
  Its portable renderer and worker/UI tests run without the Linux reference
  executable. Full differential artifact generation remains Linux-only.

