# Native Playtest presentation (phase 5)

The dialogue editor now has one Playtest action opening a dedicated window.
Restart, optional Setup and Close share the top row. Zoom, status and optional
Actions share the bottom row. Diagnostics open separately from the issues link;
the old Game preview action and snapshot tab are gone.

## Exact frame and pixels

`src/text/game_frame.c` contains the original frame drawing coordinates and
operations, shared by Impera's `ULTIMA_637e_DrawFrame` adapter and Workshop.
Workshop renders the EGA frame from the Project's normal font, then crops the
conversation region at `(185,80)` with size **135x112**. The native **128x104**
text surface sits at `(7,8)` within that crop. This preserves the actual blue and
white top/left borders. There is no invented right or bottom border, padding or
system-font substitution inside the game surface.

The independent reference retains the original frame routine in
`tests/native_frame_reference.inc`. Differential tests compare the complete
320x200 initial frame and final framed conversation crop as well as the existing
24-conversation pixel/input/state corpus. Font and conversation edits are copied
at Restart; a running session never switches resources underneath the interpreter.

## Zoom and input

The default is 300% zoom. Fixed 100–500% settings preserve their scale when the
window changes size; a scroll area accommodates a larger image. Fit scales only
the presentation to the available space. Both use nearest-neighbor drawing and
the DOS pixel aspect ratio of 5:6. Logical dimensions, line wrapping and cursor
behavior remain unchanged. Device scaling never modifies the underlying image.

The native viewport captures input, including Escape and Tab, and overrides
Workshop shortcuts while focused. Native input still owns echo, editing and
cursor animation. Opening Playtest, changing zoom and closing Setup or toggling
Actions restore focus to the game surface without injecting a game key. There
are no Qt reply fields or Send buttons.

## Lifecycle and source navigation

Setup and Actions are hidden initially. Actions retain source offsets and can
select the corresponding authoring card without consuming a game key. Editing
marks the running snapshot stale, disables its source links and shows a short
Restart status. Setup changes also require Restart. The old snapshot can continue
to play, but finishing it does not erase the pending-change status.

Restart validates/applies the current draft, cancels and joins the previous worker,
clears its images/traces, and starts with current Project fonts, conversation and
Setup choices. Invalid drafts remain editable; missing/corrupt resources report
errors outside the native viewport. Closing Playtest or leaving the editor wakes
pending waits, joins the worker and invalidates queued frames/actions. Reopening
starts a fresh execution.

## Validation

- Independent engine/reference artifacts match after the frame extraction.
- Workshop's full frame and conversation crop match native artifacts exactly.
- Fixed zoom, Fit, narrow windows and 100%, 125% and 200% device scaling preserve
  logical pixels and avoid interpolated colors.
- UI tests play an edited conversation with edited fonts: the old session retains
  its original resources, and Restart uses the new resources. They also verify
  source-link invalidation, focus, scrolling, native Escape, close/reopen and
  cancellation on leaving the editor.
- The standalone Qt-only build remains supported. Broad platform/package smoke
  tests and the expanded original-dialogue corpus remain phase 6 work.
