# Dialogue quotation and action layout audit

This audit uses the original `CASTLE.TLK`, `DWELLING.TLK`, `KEEP.TLK`, and
`TOWNE.TLK` resources and the native interpreter in `src/talk.c`. Action operands
and question definitions are excluded from the counts of executable actions.

## Quotation marks

The interpreter adds an opening quote when dispatching most spoken responses and
adds a closing quote when that response returns normally. Description responses
instead start with `You see `. Name responses start with `"My name is `. An action
that ends or transfers the conversation can bypass the normal closing quote.
Question prompts and answers have their own dispatch paths. This is not a rule
that blindly surrounds every stored entry.

Middle quotation marks are authored text. They indicate where speech stops and
resumes, and are not interchangeable with paragraph breaks or timed actions.
The original dialogue includes narration between spoken paragraphs. For example,
KEEP conversation 17 closes a spoken invitation to eat, pauses, then prints
`You put the morsel in your mouth...` and describes the food without surrounding
quotes. It resumes speech with `"Good, huh!`. Adding quotes to every paragraph in
the engine would turn that narration into something the NPC said. Quotes inside
speech and passages continuing across a wait also need their literal punctuation.

Workshop therefore preserves authored quotation marks. A future authoring feature
could explicitly distinguish Speech from Narration and generate boundary quotes
when exporting; it must allow literal quotations and preserve imported punctuation.
The engine should not infer this distinction from newline or action placement.

## Action layout

The following counts allow an inventory change or an `If introduced` conditional
between the text layout and the listed action. Conditional transfers still retain
their original position and operands; they are not reordered.

| Action | Two preceding newlines | One preceding newline | Other context |
| --- | ---: | ---: | --- |
| Wait for key | 224 | 0 | 1 narration passage with its newlines after the wait |
| Pause | 155 | 1 | 10 pauses at the start of a response |
| Enter question | 67 | 0 | 280 with no immediate text; 1 follows a lone quote |
| Ask player's name | 26 | 0 | 22 after another action or at the start of a response |
| Attempt party joining | 13 | 0 | 0 |
| Return to ordinary topics | 41 | 0 | 7 after Pause or Wait for key |
| End conversation | 4 | 86 | 3 after another action or without preceding text |

The earlier claim that all 225 waits have preceding paragraph spacing was
incorrect. The food-tasting passage in KEEP conversation 17 performs an inventory
change and waits immediately after unquoted narration, then starts a new paragraph.
Imported resources keep this placement intact.

Workshop manages the conventional layout at action boundaries:

- Existing paragraph spacing before Pause, Wait, Enter question, Ask player's
  name, Attempt party joining, and Return to ordinary topics is hidden in the
  speech field and retained in the encoded resource.
- Existing one- or two-line spacing before End conversation is retained and
  hidden. The single newline before the exceptional brief Pause remains visible.
- New handoff actions after text receive the conventional spacing: two newlines,
  or one newline for End conversation. Existing trailing newlines are reused.
- A handoff following an already-spaced Pause or Wait does not receive another
  blank paragraph. A branch-only response or lone quote receives no new spacing.
- New Paragraph Pause supplies paragraph spacing; Brief Pause supplies none.
- Moving or removing an action also moves or removes its managed spacing.

Other actions have mixed or inline uses. Inventory, payment, karma, rune controls,
guards, and conditional branches do not automatically introduce paragraphs.
`Explicit line break` remains available for deliberate custom layouts.

Paragraph menus occupy the same row as their text field. Action menus remain on
the same row as their action summary.
