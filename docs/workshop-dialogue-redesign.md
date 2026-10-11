# A conversation-first dialogue editor

## Recommendation

Replace the permanent upper editor/lower preview split with one conversation authoring surface. Keep the NPC browser and conversation outline; open Playtest or Game preview only when requested. Use readable native text for authoring, compact action cards for behavior, and the DOS font for an optional game preview.

All four implementation steps are now implemented on `feature/native-modding`, using the existing lossless dialogue model and simulator. The following sections retain the design rationale; the implementation notes below describe the actual controls and preview limits.

## Implemented interface

- One authoring surface replaces the permanent editor/preview split. **Topics** collapses the outline. **Advanced…** opens tagged source and changes to **Back to conversation**; ordinary editing has no Apply button.
- Speech uses expanding paragraphs. Avatar-name and rune controls appear as protected inline chips inside speech. Rune spans are tinted and italicized for authoring. Typing, cut, accessibility edits and ordinary paste cannot silently remove those controls; their context menu supports explicit insertion/removal. External clipboard text is always literal speech. Action cards have a menu for typed copy/paste, insertion, movement and removal.
- Keyword chips wrap within the column and provide edit/remove menus and **+ Keyword**. Shared responses and stored matching order remain intact. Topic/answer operations are enabled for the applicable selection.
- Each question shows its prompt, ordered expandable answers, **Otherwise**, and links to its incoming references together. Its fields commit as one validated document transaction; an invalid field cannot partially commit other fields. Numeric labels/offsets remain in advanced details.
- **Game preview**, **Playtest**, and the linked issue count open a separate closable tools window. Playtest’s **Setup** is hidden initially; **Show actions** reveals execution traces. Clicking a trace opens and highlights its source paragraph/action. Stale traces are cleared when the document changes.
- Valid visual edits apply after typing or when navigating. Incomplete drafts remain visible with validation feedback. Source edits use **Validate and apply source** or validation when navigating. Workshop owns Undo/Redo; untouched tokens, operands, tails and unrelated spans retain their original bytes.

### Verified rendering and remaining limits

The game preview uses IBM.CH and RUNES.CH, eight-pixel glyph rows, automatic response wrappers/quote suppression, dictionary spacing, Avatar substitution and rune toggles. Its 18-column word buffering and glyph coordinates are checked against the actual `TALK_04e2_PrintWord` and `TALK_0574_BuildOrPrintWord` functions extracted into the test harness, including their full-buffer behavior and newline suppression.

It is a selected-response snapshot starting at column zero in normal font, rather than a complete game-screen emulator. It shows the response across scrollable rows, not the running game's scrolling window or timing. Unconditional exits stop the snapshot; conditional branches use Setup state and direct the author to Playtest for continuation. Payment/world effects and waits have explanatory notes. A missing runic font is explicitly reported and tinted rather than silently treated as accurate. Guard/recruitment/world effects remain simulated traces, not engine execution. Existing engine-matching tests still verify keyword behavior.

All these changes leave the game’s TLK and mod-package formats unchanged.

## What is confusing today

Inspection of `workshop/dialogue_editor.cpp` shows two competing tab groups. The upper group contains Text & Actions and Advanced source. The lower group contains Readable preview, DOS font preview, Diagnostics and Test Conversation. The readable preview repeats the selected entry's text and action names, but cannot be edited. It permanently occupies two fifths of the editor/preview layout's stretch allocation. A separate Apply button sits alongside automatic delayed application of valid edits.

Each text fragment is also a separate boxed editor with its own heading, move buttons and Remove button. A short response interrupted by a pause becomes multiple large panels. The conversation outline offers topic and reply operations regardless of whether the current selection supports them. Question prompts, answers and fallback responses are spread across separate selections.

The DOS preview currently uses the original font and approximate 18-column wrapping. It explicitly does not reproduce automatic quotes, pagination or rune spans. The readable preview represents action names; it does not demonstrate their conditional execution. Neither should be presented as the exact player experience.

## Designs considered

| Design | Strength | Drawback | Recommendation |
| --- | --- | --- | --- |
| A single plain text editor | Familiar, compact | Hidden actions or editable control tags force authors to understand bytecode | Insufficient on its own |
| Editable DOS screen | Immediate visual resemblance to the game | Narrow text column and retro font make long responses and branches hard to author; controls still need another surface | Use as an optional preview |
| Node graph of every response | Makes routing visible | Large graphs become cluttered; cycles, aliases and ordinary topics make simple edits cumbersome | Possible later read-only flow view |
| Conversation outline plus text and compact action cards | Readable speech, visible behavior, bounded implementation using the current model | Requires careful selection, clipboard and undo behavior | Recommended |

## Default layout

```text
NPC browser       Conversation outline       Selected response
                  Basics                     Job / Work
                  Topics                     When asked: [job] [work]
                  Questions
                                             We tend the gardens of the abbey.
                                             [Wait for a key]
                                             Seek Chamfort in Yew.
                                             [End conversation]

                                             Insert…     Playtest     Game preview
```

Use the existing global NPC browser without adding another NPC list. The outline remains a resizable secondary column. The response fills the remaining height. On narrow windows, make the outline collapsible; do not permanently open a preview beside or beneath the text.

Retain Basics, Topics and Questions, but use purpose-oriented labels: Name, Description, Greeting, Job / Work and Farewell. Explain engine-added wording next to Name/Description only when relevant. Greeting help must reflect introduction/name-announcement behavior, rather than imply it always opens a first encounter.

Topic keywords appear as editable chips with an Add keyword control. Shared keywords visibly lead to one response. Ordered keyword matching is preserved; moving a topic is an explicit operation with warnings where prefixes overlap. A brief matching explanation belongs in help or validation, not in every response.

## Editing speech and behavior

Text appears as directly editable paragraphs without a Text group box. Paragraphs grow with their content instead of using fixed-height inner scrollbars. Keep actions between the exact text fragments where they execute; do not move all behavior into a properties sidebar.

Use two kinds of controls:

- Inline tokens for substitutions and formatting, such as Avatar name, rune spans and explicit line breaks. They are protected model objects, not angle-bracket strings.
- Compact cards for execution: Wait for a key, End conversation, Ask player’s name, Attempt payment, Give item, Enter question, and conditions supported by the engine.

A card normally shows a readable summary, such as “Attempt payment: 4 gold” or “If karma is at least 50 → Resistance password”. Selecting it opens its small parameter form. Forms use real engine inventory choices and named question destinations. Unknown operands remain explicit compatibility values. Failure behavior is available alongside the relevant action: payment can return to ordinary topics; question routing does not resume the remaining response.

Insert… offers Text, Variable/Formatting and Action. Show move/delete controls on the selected card or via its context menu instead of repeating them everywhere. Every operation must also be keyboard-accessible. Do not rely solely on dragging or hover controls.

First implementation should keep editable paragraphs and action widgets in one ordered document surface. A single rich-text widget with embedded interactive forms would require considerably more custom caret, selection, accessibility and clipboard work. It can be considered after the simpler surface is proven.

Clipboard policy: copying text produces readable text; copying a selected action within Workshop preserves its typed action data. Pasting external text cannot silently create instructions. Deleting an action token must be explicit and undoable. Adjacent text fragments may be displayed continuously, but untouched encoded bytes must remain intact.

## Questions as a complete editing unit

Selecting a question opens its prompt, ordered answers and Otherwise response together:

```text
Resistance password
Prompt
  What is the password of our Resistance?

Answers, checked in order
  [dawn]       [Edit response]    [Move] [Remove]
  + Add answer

Otherwise
  I do not yet believe thee!
  [End conversation]

Used by: Avatar question / answer “yes”
```

Expand the selected answer in place. Keep the fallback visible or clearly accessible; do not hide it behind a technical “Any” label. Each answer can contain the same text/action sequence as an ordinary response. Referenced questions use descriptive project names; numeric labels belong in advanced details. Existing label limits and the structural ending remain enforced.

Unrecognized structures are preserved. Disable unsafe visual operations on those sections and offer an explicitly advanced source view with an explanation. Do not guess how malformed input should be repaired.

## Preview and playtesting

Remove Readable preview: the authoring surface already serves that purpose.

Game preview opens a closable dock or popover. It renders the selected response using the original font and target text column. Clearly label remaining rendering approximations until quotation, word spacing, rune mode, pagination and substitutions have been verified against the engine. Never claim an exact screenshot from the current approximate font renderer.

Playtest opens an optional dock with conversation transcript and player input. Hide sandbox configuration behind Setup: Avatar/party names, introduced state, gold, karma and party capacity. Keep Reset visible. Put executed actions in an optional trace, separate from NPC speech. Clicking a trace entry should navigate to its source. Existing simulator limits and unsupported world effects remain visible where relevant; the test cannot mutate the mod or saves.

Keep one compact validation summary, e.g. “2 warnings · 617 / 1024 bytes”. Clicking it opens actionable issues and jumps to the affected response/action. Show draft errors near the offending control. Valid edits apply to the project automatically; remove the ordinary Apply button. Source editing can retain an explicit Validate and apply action for transactional reparsing.

## Implementation sequence

1. **Simplify the layout.** Remove the duplicate readable preview and persistent lower pane. Make Game preview, Playtest and validation optional. Keep the existing block editor and codec initially; remove the redundant ordinary Apply control. This delivers useful improvement with the least model risk.
2. **Build the conversation surface.** Replace text group boxes with expanding paragraphs and compact action cards. Add contextual insertion, keyboard navigation and grouped undo; retain all original instruction order and untouched bytes.
3. **Unify question editing.** Present prompt, ordered answers, fallback and incoming references together. Implement keyword chips and contextual outline operations without changing matching precedence.
4. **Improve fidelity and interaction.** Add safe inline variable/rune objects, source-linked playtest traces and verified game rendering. Keep source behind an Advanced command rather than a permanent competing main tab.

## Acceptance checks

- A simple Job response has one obvious editable text surface and no repeated preview text.
- A text/wait/text/end sequence remains ordered, readable and keyboard-editable.
- Conditional introduction, karma branches, payment, name recognition, shared aliases and question cycles retain their engine semantics.
- Existing original conversations reopen/export byte-identically when unedited; editing one text fragment preserves unrelated instructions, operands, tail and padding.
- Invalid drafts survive switching views without silent loss; export rejects unresolved errors. Undo/Redo restores text, action parameters and structural edits with sensible operation grouping.
- Question deletion requires resolving incoming references; moving answers preserves their ordered matching behavior.
- Test sessions remain bounded and isolated. Preview limitations are explicit, and preview/test windows can be closed to restore authoring space.
- The NPC browser still excludes schedule-only NPCs. NPC schedule editing remains available through maps.
