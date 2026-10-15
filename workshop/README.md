# Impera Workshop — native mod authoring (first version)

Impera Workshop is a C++17 / Qt 6 desktop application, not a Python wrapper.
It reads original DOS Ultima 5 resources, keeps edits in a project, and exports
**`.imperamod` packages**. Neither editing nor exporting rewrites the game files.
The app is an initial implementation; it is not yet a finished replacement for
all of the specialized editor interfaces.

## Build and run

Install a C++ compiler, CMake 3.16 or newer, and Qt 6.2+ Widgets development tools.
On Ubuntu/Debian, the additional development package is `qt6-base-dev`.
On Windows/macOS, use the Qt installer and set `CMAKE_PREFIX_PATH` to its kit.

Build Workshop independently (SDL and Python are not required):

```sh
cmake -S workshop -B workshop-build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build workshop-build --config Release --parallel 3
ctest --test-dir workshop-build -C Release --output-on-failure
```

The executable is `workshop-build/ImperaWorkshop` on Linux,
`workshop-build/Release/ImperaWorkshop.exe` on Windows, or
`workshop-build/ImperaWorkshop.app` on macOS. Development builds require Qt runtime libraries. GitHub releases also provide
one combined download with Impera and Workshop for all four platforms, with Qt
bundled and an empty external `Mods` folder. Linux archives contain both AppImages;
Windows ZIPs contain both executables; macOS archives contain both app bundles. Launch
`Run Workshop.cmd` on Windows, `ImperaWorkshop.app` on macOS, or the executable
AppImage on Linux. Release deployment uses Qt 6.8.3 and
`-DIMPERA_WORKSHOP_DEPLOY_QT=ON`; normal local builds still support Qt 6.2+. The
Windows resource, macOS bundle and embedded window icon use Impera's artwork.

Alternatively, add `-DIMPERA_BUILD_WORKSHOP=ON` to the engine's CMake configure
command. Normal engine builds do **not** need Qt. CI builds and tests the native
application on Linux x86_64, Windows x86_64 and both macOS architectures.

```sh
workshop-build/ImperaWorkshop --game '/path/to/Ultima 5'
workshop-build/ImperaWorkshop --project my-mod.imperaproject
```

## Authoring workflow

1. **New Mod**: choose the original DOS game directory. Workshop does not load
   installed mods into the base game; you can explicitly import a package.
2. **Name Mod**: give the project a name used in Impera's diagnostics.
3. Workshop opens Britannia centered on Lord British’s Castle. Choose locations or other content from the sidebar and edit it.
4. **Save Project** creates `.imperaproject` JSON containing the original folder
   reference/checksums and changed resources. Keep this project for future editing;
   it requires the same original files when reopened. It is not a distribution file.
5. **Export Package** creates a sparse, validated `.imperamod` file.
6. Copy the package into **`<Impera application folder>/Mods/`**, then restart Impera.
   Create `Mods` if it does not exist. To disable a mod, move its package out and
   restart. There is no in-game mod-selection UI yet.

Projects are your working documents: they retain source-file verification and
Workshop-only metadata such as dialogue question annotations. Packages are the
files you distribute and install in Impera; they contain only runtime mod data.
You can import a package to continue editing its game content, but it does not
restore project-only annotations. Keeping a project is recommended, not required
for running a mod.

The sidebar is always available. **Tools → Find Content** (Ctrl+L) focuses its
search; there is no separate Content toolbar button. Map thumbnails appear only
for Britannia and the Underworld. Map controls retain tooltips, but the canvas
has no instructional hover popup or permanent instruction strip.

Dialogue editing has one main surface. Use **Topics** to hide/show the outline,
**Game preview** or **Playtest** for optional tools, and **Advanced…** for tagged
source. A question’s prompt, answers and Otherwise response appear together.
Avatar/rune chips are protected: their context menu explicitly inserts or removes
controls; ordinary paste produces speech rather than instructions. Action menus
support typed action copy/paste. Valid edits apply automatically and support
Workshop Undo; invalid drafts remain visible until corrected or explicitly discarded.
The [dialogue redesign notes](../docs/workshop-dialogue-redesign.md) explain preview
verification and the remaining differences from running the game.

Keyboard: **Ctrl+N** new project, **Ctrl+O** open project, **Ctrl+S** save,
**Ctrl+Z** undo, **Ctrl+Shift+Z** redo, **Ctrl+E** export. Qt uses the usual Command
shortcuts on macOS for standard editing/project actions. Dialogue edits apply after a brief typing pause; **Apply** commits immediately.
Story edits use **Apply**. Navigation, saving and export check invalid or unapplied
drafts rather than silently discarding them. Package imports become undoable edits.

## Workspaces

| Workspace | Available in this first version |
| --- | --- |
| Maps | Searchable world/settlement/combat navigation; continuous painting, synchronized eyedropper, hover/brush preview, pan, anchored zoom, fit view, coordinate navigation, per-map view restoration and per-stroke undo; terrain selection, solid/outline rectangles, bounded flood fill, typed copy/paste previews, minimap, category-filtered tile browser, original-change highlighting; terrain and tile-ID PNG exports |
| Dungeons | Eight named dungeons with eight 8×8 levels; symbolic feature painting, variants, encounter-room links, floor stack, ladder warnings, undo and byte-preserving edits |
| Signs | Map markers, location-specific floors, search, add/move/delete records, mixed normal/runic lettering and glyph preview; generated posters are identified |
| Descriptions | Searchable terrain/object and actor thumbnails, independent edits of shared text, explicit linked edits and DOS preview |
| Settlement NPCs | NPC mode with actor selection, overlap chooser, drag previews and atomic moves; schedule inspector, signed floors, other-position ghosts, conversation links and advanced record editing |
| Combat setup | Edit starting party positions for four directions, monster tiles/positions, triggers and changed-tile metadata; retain all padding/sentinel bytes |
| Graphics | All DOS `.16` image containers and 512 tiles; thumbnail gallery, EGA pixel painting, PNG import/export, full tilesheet import/export, one-bit alpha masks |
| Conversations | Dialogue-only NPC lists; Basics/Topics/Questions outline; expanding speech with protected inline Avatar/rune controls; keyword chips and compact action cards; unified prompts/answers/fallbacks; reference-safe question deletion; optional game preview, source-linked playtest and diagnostics; lossless Advanced source and project Undo |
| Narrative | 30 creation entries with virtue choices, six illustrated ending pages, karma context/sample values, 194 shop entries with token substitution; fixed-offset or versioned extended text, byte budgets, font previews and undo |
| Story | Twenty fixed-offset text pages; original capacity, offsets, padding and terminators retained; normal, runic and proportional font previews |
| Starting state | Party position, time, supplies, plot items, reagents, moonstones, shrine/dungeon flags, Shadowlord settings; sixteen character records, statistics/equipment, safe party joining/removal |
| Resource overview | Read-only information for resources without supported authoring controls; no hexadecimal editing |

### Signs and Look descriptions

Open **Signs** in the resource library, or **Signs layer** from a settlement map.
Choose a location and one of its actual floors. Click a sign marker or list entry
to inspect its coordinates and text. Click another map cell (or use the arrow
keys), then **Add sign at selected cell** to create a record. Coordinate changes
move the record, and **Delete sign record** removes it after confirmation. These
operations never paint terrain: the inspector warns if the selected tile does
not read sign text with the Look command. World signs are available on Britannia.

Plain ASCII text uses normal lettering. **Runic lettering** wraps the selection
in `[runes]…[/runes]`; preserved `[byte:NN]` tokens retain decorative glyphs,
border shortcuts and pauses without changing original text. The preview uses the
project's normal/runic fonts, the engine's border expansions and its 16-column
text window. A legacy blank sign can lead into the following record at runtime;
its isolated preview shows only its stored text. Pauses are not interactive in
preview. The Eight Laws fallback and Yew's wanted poster are generated by the
engine; the poster's overridden file record is preserved but not offered for
editing. Apply commits one undoable edit. Resource navigation, save and export
apply valid drafts; switching text entries or location floors asks before
unapplied text is discarded.

Open **Look descriptions** to edit the terrain/object or actor description table.
Search names/text and select a thumbnail. The inspector shows how many entries
share the text. Editing normally detaches only the selected entry; explicitly
check **Change all entries sharing this text** to update all aliases, even across
the two tables. Use **Insert line break** for a new line. Text is limited to 127
ASCII bytes. Special well/fountain interactions and other engine-generated Look
responses can override these descriptions. Export retains unused text-pool bytes
and unchanged records; it never automatically deduplicates descriptions.

### Map workspace controls

Use the Maps tree or page selector to switch locations/floors. The tile browser
accepts decimal or hexadecimal IDs and shows the active brush separately. Maps
use edited project artwork from `TILES.16`; only terrain IDs 0–255 are paintable.

- Left-drag paints one continuous, undoable stroke; right-click picks a tile.
- **B** selects Pencil and **I** selects Eyedropper while the map has focus.
- Middle-drag, **Space+left-drag**, or the Pan tool moves the view without editing.
- **Ctrl+wheel** zooms around the pointer; **+ / −** zoom around the view center.
- Wheel scrolls vertically; **Shift+wheel** scrolls horizontally. Scrollbars remain available.
- Choose **Fit** for the whole map, or an integer zoom for detailed work. Fit uses crisp nearest-neighbor rendering.
- **Escape** or focus loss cancels the entire pending stroke. A failed edit rolls back its preview.
- **Go** centers the entered X/Y cell. Brush, zoom, tool, schedule slot and view center are remembered separately per map page during the session, including across undo/redo.
- **Back to map** returns from other resource editors without losing map navigation.
- **Export** distinguishes terrain PNG from tile-ID PNG; NPC overlays are excluded.

Terrain tools are map-local: **V** selects a rectangle, **R** paints a rectangle,
and **F** fills four-connected cells of the clicked tile ID. A selection constrains
Pencil, Rectangle and Fill; **Escape** clears it when no gesture is pending.
Selection bounds and dimensions appear beside the map.

Use platform-standard **Copy / Paste** shortcuts (Ctrl on Windows/Linux, Command
on macOS) while the map has focus, or the toolbar buttons. Copy includes only
terrain IDs. Paste previews its changed-cell count and accepts placement with a
left-click; it rejects placement outside map bounds rather than clipping, and
**Escape** or focus loss cancels it. Paste can place terrain outside the source
selection or into another map; it never copies NPCs or combat records. Each
terrain operation is one undo command; rejected operations leave resources intact.

Click/drag the **minimap** to navigate; the white outline shows the viewport.
**View → Highlight changes** marks terrain differing from original game files in orange,
with a changed-cell count. The browser offers **All tiles / Overworld / Ground /
Buildings / Objects / Other**. Overworld includes tiles actually used in Britannia
or the Underworld, and can overlap one of the four artwork categories.
Selection, comparison and rectangle mode also survive map navigation and undo.

### NPC mode

Choose **NPCs** in the map tool selector to work with existing NPC records. Click
an actor or select its slot in the inspector. Overlapping actors open a chooser
when none is selected; subsequent drags move the selected actor. The record list
also lets you select another actor sharing that tile.

Drag previews a destination; release commits one undoable move. **Escape**, focus
loss or releasing outside the map cancels it. NPC mode protects terrain, including
paste. Moves change only the selected location's X/Y/Z, preserving other
positions, AI, hours, appearance, dialogue linkage and unknown bytes.

The inspector resolves names from the companion conversation file, displays all
four real transition hours, and shows raw AI values. **Slots 1 and 3 share location
record 1**: moving either changes both. Floors are signed; **−1** is a basement.
**Locate on map** navigates to the selected position's floor. Enter X/Y/floor and
use **Move here** to relocate, including between a settlement's existing floors.
**Other schedule positions** shows numbered slot ghosts on the current floor,
with other-floor positions listed in the inspector. These are stored positions,
not a simulation of wandering NPCs.

**Edit conversation** opens the linked dialogue by its ID; **Back to map** restores
actor, schedule slot and viewport. **Advanced NPC record** opens the existing
record editor for AI, transition hours, appearance and dialogue ID. Missing
conversations disable the link; unusual stored positions remain visible in the
inspector without being clamped. No NPC creation/deletion is added in this phase.

Graphical combat-setup editing is the next phase. View preferences do not change
game bytes or get exported into a mod package.

Basements use the engine's actual floor numbers. Britannia changes rebuild both
`BRIT.DAT` and the chunk-index bytes of `DATA.OVL` together; implicit-water and
shared chunks are handled. More than 255 distinct non-water chunks is rejected.
Underworld and settlement extension bytes are retained.

Artwork must use the exact DOS EGA palette. Alpha is binary; `TILES.16` has no
alpha channel. Alternative-platform/custom PNG tilesets in Engine Options are
independent of the DOS graphics resources edited here.

Conversations use an NPC list, a conversation outline and a detail editor. Edit
normal text without compression tags; unchanged blocks keep their exact original
bytes. Actions have explicit amount, inventory and question-destination fields.
Topics and replies can be reordered, duplicated, added or deleted. Comma-separated
keywords share one response. Question names are project-only annotations. Deleting
a referenced question requires retargeting its callers. Undo/redo includes edits
and annotations.

Diagnostics identify missing destinations, invalid operands, byte limits,
overlapping keywords and other problems. Existing original oddities remain
loadable and are retained as raw sections. **Advanced** exposes lossless tagged
source for the selected entry or explicitly the whole conversation; unknown bytes
remain available there. `<Entry>` separates entries, `<New Line>` is an in-game
line break, word tags retain compression and `<Byte N>` exposes exact bytes.
Labels and action operands retain the original DOS semantics.

**Test Conversation** runs in a bounded sandbox with configurable party names,
gold, karma and introduction state. Click a trace row to inspect its source.
Inventory/world effects are traced rather than applied, and recruitment is
reported rather than fully simulated. The DOS preview uses the selected game's
original bitmap font, with approximate 18-column wrapping; it is not a complete
emulation of the game's quotation, pagination or rune rendering. Testing never
modifies the game or project. See [the dialogue design](../docs/workshop-dialogue-ui.md).

Starting-state mods primarily affect **new games**. The original character-
creation sequence still overwrites the fields it traditionally initialized.
Existing saves retain saved state, object lists and NPC data. Initial world
object overrides seed a fresh runtime save directory; they do not overwrite an
existing player's object files. Removing a mod does not undo changes already
captured in a saved game.

See [the map workspace UI plan](../docs/workshop-map-ui.md) for the proposed
canvas, terrain tools, NPC schedule inspector and combat placement interface.

## Scope and next work

This branch establishes native editing and end-to-end package loading. It covers
the audited formats and supports bringing existing editor outputs into packages.
An optional graphical dialogue flow diagram and interactive combat-entity
placement tools remain future work; the conversation outline and combat tables
provide structured access now. There is no flood fill, multi-tile selection, plugin scripting,
package dependency system, runtime hot reload, or mod manager yet. Unsupported resources have a read-only overview; unknown fields are preserved.
Windows/macOS CI is configured; local interactive validation was on Linux only.

## Validation

Native tests cover compression width/reset boundaries **against the actual C
engine decoder**, paired graphics offsets/masks, PNG alpha/palette rejection,
dialogue token/control roundtrips, story offsets, map chunk allocation,
projects/packages/checksums and widget loading without mutation. Set
`U5_GAME_DIR` to your original game directory to include optional full-resource
and all-workspace checks. No game assets are included in generated test fixtures.

```sh
QT_QPA_PLATFORM=offscreen U5_GAME_DIR='/path/to/Ultima 5' \
  ctest --test-dir workshop-build --output-on-failure
```

The engine also has a `mod_packages` CTest covering corrupt/truncated packages,
all-or-nothing mounting, load order, conflicts, reloads and read/write isolation.
See [the package format](../docs/mod-packages.md) for the shared engine contract.


Combat maps now include **Encounter** mode: choose North/East/South/West entry,
select party starts, monsters or triggers from the map or record list, and drag
to move with one undoable edit. Escape cancels a drag. Trigger selection shows
both linked cells and the replacement tile; **Preview selected trigger result**
is read-only. **Combat setup** retains raw and unusual metadata values.

The game-files directory is remembered across startups. Explicit `--game` or
`--project` arguments override that preference. If the remembered folder moves
or becomes invalid, use **Open game / New mod** to select it again.


### Map editor polish

Find terrain tiles by name or number and filter by **Overworld**, **Ground**,
**Buildings**, **Objects**, or **Other**. Overworld includes tiles used in the
Britannia and Underworld maps and their animation frames. Overworld can overlap
Ground, Buildings, Objects, or Other; those four base categories remain exclusive.
All 256 terrain tiles are covered by the
[artwork and category audit](../docs/workshop-tile-catalog-audit.md). The compact
palette shows artwork only; hover for names, categories, and IDs. Tool icons have
descriptive tooltips. The labeled NPC schedule selector beside the floor list shows shared start hours
or changes 1–4 when NPC timings differ.
On the map canvas, use arrow keys to target a tile and Enter to apply the tool
(or move the selected character/encounter marker). Shift+arrows extend a terrain
selection; standard Copy/Paste and Escape work alongside B/I/V/R/F tool shortcuts.
These shortcuts do not intercept typing in inspector fields.

**Terrain PNG** exports stored terrain artwork; **Tile-ID PNG** exports one
numeric tile ID per pixel. Both exclude NPCs, markers, grid and trigger previews.
**Visible preview PNG (includes overlays)** captures only the visible canvas,
including its current authoring overlays, at the current zoom.

NPC schedules have **four schedule changes** and **three destinations**. Each
change has its own start hour. Changes **2 and 4 share a destination and behavior**,
so moving one moves both. Their start hours remain independent. The map inspector
shows actual start hours and offers a Start hour field; unusual existing values
are preserved. Advanced schedule details explain the underlying behavior and
appearance codes. The editor does not simulate the character's runtime movement.

### Validate and test a mod (Phase 6)

Open **Mod Tools → Validate / Package Preview** to review replaced resources,
format errors, unusual schedules and conflicts with installed packages.
Double-click a resource to inspect it. Export Package shows this report first;
errors block export, while warnings preserve unusual values. Impera rejects
packages that overlap resources in an earlier package; it does not merge them.

Use **Set Impera Executable** once, then **Test Mod → Launch isolated test**.
Select the native Impera executable, Linux AppImage, or macOS Impera app/native
engine. Workshop copies game resources into a unique private test folder and
loads only the generated mod. Normal Mods, saves and settings stay untouched.
Create a new character to test changes to the starting game state.

**Open Last Test Folder** opens the retained saves and logs (`LOG.TXT`,
`engine-output.log`, `engine-errors.log`); older Linux launchers may put runtime
files in its `impera` subfolder. **Stop Mod Tests** requests that test processes
exit. Closing Workshop also stops remaining tests. Delete old test folders when
you no longer need their logs/saves. Distribute the `.imperamod` file, not these
folders containing copied game data.

**New Mod** reuses the remembered game folder. Use **Mod Tools → Change Game
Folder** when you actually want to select a different installation.

See [the implementation plan](../docs/workshop-map-ui.md) for the isolation and
validation contracts.

Copy, Paste, Clear Selection, and Highlight Changes share the tool icon row.
Zoom sits beside the tool icons; Grid and Rectangle Outline controls are removed.

See the [Other Resources audit](../docs/workshop-other-resources-audit.md) for
all 18 remaining resource files, proposed editors, and runtime compatibility
limits affecting original DOS data tables and world-object state.

The [resource editor implementation plan](../docs/workshop-resource-editors-plan.md)
covers the proposed editors, user interface, runtime compatibility fixes and
phase-by-phase acceptance checks.

World-map exports now use their authored Britannia chunk index in Impera's normal
and expanded viewports. Starting object edits require a new game; `UNDER.OOL`
overrides `INIT.OOL`, and existing saves retain their own objects. Package Preview
explains these rules and warns about unsupported DOS overlay table edits. See
[world-resource compatibility](../docs/mod-packages.md#world-maps-and-starting-objects).

### Shared resource editor foundation

The Resource Library uses friendly names for audited resources. Search also accepts
original filenames. Resources awaiting dedicated editors show their availability
and known entries; double-click an entry to inspect its byte span. Advanced byte
edits use the same undo history and project/package storage as visual editors.
Unknown layouts remain inspectable without rewriting surrounding bytes.

Story previews can use normal, runic or proportional game fonts. Non-ASCII
characters are highlighted rather than converted silently. Proportional previews
need the original `PROPORT.PCS` in the game directory; Workshop reads it without
including it in a package. Font previews are approximate and do not reproduce full
scene layout or expand format-specific control tokens.

### Dungeon workspace

Open **Dungeons** in the Resource Library's Maps group. Select a dungeon and
Level 1–8; Level 1 is nearest the surface. Paint symbolic features with the shared
Pencil, Rectangle or Fill tools, or use Select to copy a feature area. Right-click
picks the full feature byte, including its variant. Dungeon selections cannot be
pasted into ordinary terrain maps or vice versa.

Choose a brush feature, then its variant: fountains can cure/heal/poison, fields
can induce sleep or poison gas, and room doors select Room 1–16. Advanced variants
preserve the original format's less common bits. **Apply brush to selected cell**
configures the inspected position. The floor stack shows the same X/Y position on
all eight levels; click a level to inspect it. Dungeon edges wrap.

**Open combat room** opens the linked encounter map and setup editor.
**Back to dungeon** restores the dungeon, level and cell. Deceit and Despise share
a room bank in the original game; modifying a shared room affects both.

Unmatched ladders are warnings. Missing combat rooms, truncated layouts and newly
introduced impossible features block export. An already-loaded dungeon in a save
can retain its old layout; leave and re-enter to load the package's layout.

### Introduction and Ending

**Cutscenes → Introduction story** and **Ending** use the same illustrated page editor.
Select a page to edit its text and choose an original artwork layout. **Add page**
inserts a blank page after the selection; **Duplicate**, **Remove**, **Move up** and
**Move down** manage the sequence. Each sequence keeps between 1 and 128 pages.
The Introduction includes its original portal scene as an editable page.

The live 320 × 200 preview combines the project's artwork, the original proportional
font, text exclusions, justification, indents and discretionary hyphens. It shows
the completed page; transition/reveal animation remains in the game. Page edits,
navigation, save and export preserve drafts; page operations support undo/redo.

Edited sequences are stored in `CUTSCENE.OVR` (schema 1, package version 6), leaving
`STORY.DAT` and `END.DAT` unchanged. A sequence that has not been edited continues
using the original data, including existing `TEXT.OVR` ending edits. New packages
require an Impera build with cutscene page support; older engines reject them.

### Narrative

The **Narrative** library group contains **Character creation**, **Ending**,
**Karma speeches**, and **Shops**. Search the entry list by virtue, page or
service. Creation shows all 28 virtue pairs plus the introduction and completion
message, with read-only A/B outcomes. Ending lists six illustrated pages. Karma
lets you select Lord British or Blackthorn and a sample value from 0–100.
Shops includes equipment descriptions, reagent descriptions, tavern responses,
guild supplies, healer messages, shipwright messages and inn services.

**Apply text** commits an undoable edit. Navigation/save/export also check the
current draft. Other narrative fixed edits retain original offsets and show the byte allowance,
including the terminating byte; nothing is silently truncated. **Extended text**
stores a verified entry in a mod-only `TEXT.OVR` companion, retaining the original
DAT bytes. Limits remain 1,999 text bytes for creation/ending/karma and 1,499 for
shops. Shop substitution output is additionally bounded to 1,499 bytes.

Shop buttons insert `[shop]`, `[keeper]`, `[time]`, `[price]`, `[quantity]`,
`[item]`, or `[customer]`. Sample fields show the corresponding runtime values.
**Insert word** adds an original compressed-word token (`[word:NN]`); its spacing
matches the engine. `[[` escapes an opening bracket. Proportional previews support
`{` paragraph indentation and `_` discretionary hyphens. Font/layout previews
are approximate in the other narrative editors. The Introduction/Ending editor
uses the shared cutscene layout renderer. A byte-valid message can still overflow a scene; use **Test
in Impera** to verify its final appearance.

Extended-text exports use **package version 2 / narrative schema 1**. Use an
Impera build containing this feature. Existing version-1 packages and original
DAT replacements continue working. Within an accepted package, an extended entry
wins over DAT text, and entries absent from the companion use DAT text. Installed
packages still follow the existing whole-resource conflict rule: the first valid
package in filename order is accepted; later overlapping packages are rejected,
including two companions containing otherwise different entries. The companion
cannot be merged implicitly.

Previously released engines reject version-2 packages instead of ignoring their
text. Their existing diagnostic is “Malformed or truncated mod package”; this
change cannot improve messages in binaries already distributed. New engines
identify unsupported package versions and narrative schemas explicitly.

Developer audit: `scripts/generate-narrative-tables.py` derives the catalogue,
virtue outcomes, ending artwork placement and shop dictionary from the engine.
Run `python scripts/generate-narrative-tables.py --check` after changing those
tables. Python is not a Workshop build or runtime dependency.

### Special scenes

The **Scenes** library group contains **Special scenes**, plus direct access to
its message documents. Select Blackthorn imprisonment, Shrine ritual, Codex
interaction, Final rescue or Character creation. **Map** paints stored terrain
using the normal tools. Four 11×11 scene maps retain their row padding; creation
has four 19×4 backgrounds named The Summoning, The Journey, The Arrival and The
Welcoming. Other backgrounds, reserved bytes and trailing data are preserved.

**Messages** opens the same undoable text document as the library. Entries are
scoped to the chosen scene and linked to verified runtime consumers. Codex runic
messages select the runic preview automatically. Fixed text preserves offsets;
extended scene text uses the existing `TEXT.OVR` companion. Blackthorn and rescue
entries allow at most 999 text bytes; shrine/Codex entries allow 1,999. Scene
layout and scrolling still need testing in Impera.

**Sequence** edits character creation's verified instructions: actors, movement,
waits, moongates, background/terrain changes, visual effects, clearing actors and
repeats. Parameters, insertion, deletion and reordering are undoable. Inserting
**Begin repeat** wraps the selected step in a matched repeat block. Restart stays
last. Navigation/save/export apply valid parameter drafts; invalid drafts block
navigation until corrected. Unknown operations make the sequence read-only while
remaining byte-for-byte intact; its maps remain editable.

The preview shows static terrain and actor positions after a selected step's first
execution; later steps include completed repeats. **Go to affected background**
opens the corresponding map/cell. It does not reproduce intermediate animation,
audio, exact timing or keyboard interruption. Character creation has no text
opcode: questions are in Messages, and stage titles remain engine code. The other
four scenes' actors, branching and timing also remain fixed engine code. This is
an editor for the original scene resources, not a general cinematic scripting
system. Impera validates creation instructions before mounting a mod or running
the sequence, rejecting unsafe references, coordinates and control flow.

Extended scene messages require an engine with Workshop Phase 6 support, in
addition to package-version-2 support. Older narrative-enabled builds do not have
these scene entry points. Ordinary DAT edits use the existing version-1 format.

### Fonts, starting objects and engine tables

Use the [bitmap font editor](../docs/workshop-fonts.md) for normal, rune and
border previews; Hercules fonts are clearly marked as legacy presentation data.
The [starting world objects workspace](../docs/workshop-world-objects.md) combines
Britannia and Underworld templates, placement and explicit inheritance. Existing
saves retain their objects. [Shops and location tables](../docs/workshop-engine-tables.md)
provide named, bounded controls with runtime-supported companion overrides and
a searchable inventory of remaining unsupported overlay data.

### Resource navigation

The **Resources** toolbar button (**Ctrl+L**) reopens the resource library from
every editor without discarding the current draft. Resource filenames carry an
asterisk when they differ from the original data or have an explicit override;
saving the project does not clear this original-data comparison. Undo or reverting
the resource removes the marker when it becomes inherited again. The mod title
and unsaved-project marker appear in the window title. Editors open directly;
there is no raw hexadecimal editor or separate Visual Editor tab.

In Conversations, **Location** filters NPCs using their schedule assignments across
all floors and times. It combines with text search. Shared conversations appear
in each relevant location, and **Unassigned conversations** keeps unused dialogue
accessible. Filtering leaves the current conversation draft open.

### Map contents and containers

Select a discovery or chest with **Select / Move** to edit its contents in the
right-hand pane. **Original game behavior**, **Custom contents**, and **Empty**
control whether it inherits or replaces the game's rewards. Add named items,
change quantities, choose refill behavior, and configure container locks/traps.
Settlements, both world maps, dungeon levels and encounter templates are supported.
Contents edits export package version 4 and require the matching Impera build.
See [map contents and containers](../docs/workshop-containers-plan.md) for usage,
save compatibility, encounter lifetime and current limits.

### Add map items

Use **Add…** in the map tool row to place NPCs, monsters, signs, chests or
containers. Worlds also offer horses, skiffs, frigates and magic carpets;
dungeon combat rooms offer triggers. Click to place, then use the right inspector.
**Select / Move** selects and drags existing items; **Delete** removes the selected
item. Placement, attached data and deletion support Undo.

See [adding map items](../docs/workshop-add-map-items.md) for map-specific behavior,
capacity limits, save compatibility, and the audit of other behavior-bearing tiles.

### Alternate tileset editing

**Graphics → Tilesets** includes DOS, Apple II, Grayscale, Amiga, and Sharp
X68000 terrain/actors. The pixel editor preserves each source's native dimensions,
layout and colors, with tile/sheet PNG import/export and Undo. Alternate sheets
are bundled into Workshop and exported as mod resources. In Impera, choose the
matching tileset in Engine Options to see those changes. See the
[tileset documentation](../docs/workshop-content-navigation.md#alternate-tilesets).

The map editor’s **View** menu contains only **Contents markers** and
**Highlight changes**, both checkable. Inspectors open by selecting an item with
Select / Move; the View menu does not contain inspector shortcuts.

### Shop and service dialogue

**NPCs → Shops & Services** uses a response tree, one editable text pane and a
right-hand value-insertion pane. Shop values appear as inline labels; compressed
words appear as readable text. Valid edits save with Undo and use extended mod
storage automatically when needed. Shop/service templates are shared across
providers. Prices and inventory remain in Game Rules. The shop editor has no
in-game-font preview or sample-value controls.

### Speech and narration

Speech paragraphs show quotation marks; narration does not. Narration can begin or
end a response, or form the entire response, including questions and their answers.
Workshop handles the quotation marks automatically. Narration packages use the
updated Impera dialogue extension; older engine builds reject them and request an
update. Playtest uses that same native implementation.
