# Workshop content navigation

Phases 1–6 of the overarching UI redesign replace file-first navigation with a persistent content navigator.

Choose World, NPCs, Cutscenes, Graphics, Game Rules, or Mod Overview in the workspace selector. Search filters the current workspace by content name or category. World lists settlements once; selecting a settlement opens its main floor. Other floors remain in the editor's floor selector. Combat encounters use numbered labels where their purpose has not been verified.

The navigator stays available beside every editor. Tools → Find Content (Ctrl+L) focuses its search without replacing the editor or discarding drafts. Back and Forward (Alt+Left / Alt+Right) navigate independently of Undo and Redo. Validated drafts are committed before changing destinations; invalid drafts keep the current editor open. Returning through history restores editor selections and map views.

Semantic destination IDs resolve to the existing Project resources and map pages. There is no second editable data store, and serializers and package formats are unchanged. Filenames remain available in source tooltips. Asterisks indicate changes against original data, including explicit overrides, rather than unsaved project changes.

The existing standalone map navigation remains available to standalone map workspaces; the main application hides it in favor of the shared navigator. Existing editors, contextual tabs, map-to-dialogue links, dungeon-room links and read-only compatibility information remain accessible.

## Location modes (phase 3)

Location editing uses one map canvas, floor selector, zoom and camera. There is no location-tab row. **Select / Move** opens the appropriate right-hand inspector when clicking an NPC or a sign; dragging moves its destination or sign record. The editors support:

- **Terrain:** paint, select, copy and paste terrain.
- **NPCs / Schedules:** inspect settlement NPCs, move their destinations, set schedule-change hours and open their conversations. The schedule selector appears in this mode.
- **Signs:** edit sign text, add a sign record at the targeted cell, and inspect its location. Only records on the current location floor appear. This changes text records, not terrain; place an appropriate readable sign tile in Terrain mode too.
- **Starting Objects:** add and move fresh-game objects in Britannia or the Underworld. The selected world is fixed by the shared map; existing saves are unaffected. Adding uses the targeted map cell. Explicit overrides, inheritance and undo retain their existing semantics.
- **Encounter:** place party starts, monsters and triggers on combat maps, with read-only trigger-result previews.
- **Encounter Details:** the existing advanced metadata controls, fixed to the selected combat map. Unverified/padding values remain explicitly identified.

Inspectors appear only when applicable data is available. Settlement floors remain in the shared selector; signs have no second location/floor picker, and starting objects have no second world picker. Painting tools remain visible and restore the terrain palette when selected. **Map items** provides sign creation, starting objects and advanced encounter details. Switching inspectors cannot paint terrain. The bottom coordinate controls remain; the application status bar is hidden when empty.

Switching modes or floors commits valid pending sign text first. Invalid text keeps the current mode and floor open. Undo, navigation history and map-to-conversation return retain the active location mode. Selecting a sign or object does not move the camera automatically; use Show selected sign/object to center on it. Damaged companion data shows a recoverable layer error rather than closing the application.

The standalone text and starting-object collections remain accessible for project-wide editing. Dungeon level/room editing remains its specialized workspace.

## NPCs (phase 4)

The NPCs workspace lists individual scheduled NPCs across all four conversation banks, grouped and filtered by top-level location. Search matches NPC names and locations. NPC identity includes its bank, location and schedule slot; equal conversation numbers in different banks do not refer to the same conversation. Empty appearance slots are excluded. Unassigned conversations remain available in a separate group; they may still be referenced by special game logic.

Selecting an NPC opens its conversation directly, with a compact NPC/location heading and **Show NPC on map**. Shared-dialogue assignments are available in the heading tooltip. The duplicate schedule tab and full-width dialogue-users button are removed. NPCs without editable dialogue are excluded from this browser; their schedules remain editable in the map’s NPCs / Schedules mode.

The Conversation view has one authoring surface with speech, protected inline controls and compact action cards. Question prompts, ordered answers and Otherwise responses share that surface. Preview, Playtest and validation open only when requested; see [the dialogue redesign](workshop-dialogue-redesign.md) for details.

Schedules belong to the map editor: drag NPCs to change destinations, choose the schedule change, and edit its start hour in the NPC inspector. Advanced schedule details retain behavior, appearance and conversation-link editing. Changes 2 and 4 still share their destination and behavior. **Show NPC on map** opens the first scheduled floor with the NPC selected.

Valid conversation drafts commit before switching view or navigating; invalid drafts prevent leaving. Undo and Back/Forward preserve NPC identity and the selected response. Navigation rebuilds preserve the list’s scroll position and expanded location groups, including when a pending draft commits on focus change. NPC navigation labels omit spoken introduction closing quotes without changing dialogue bytes. The existing conversation editor retains topics, questions, preview, simulation and validation. Compatibility routes for damaged or empty banks remain available. No resource or package format changes are introduced. Change markers identify the selected NPC’s schedule and conversation; shared conversation edits mark every NPC that uses it.

## Artwork, stories, rules and new games (phase 5)

Graphics lists Tilesets first, followed by Illustrations and Fonts. Artwork collections use verified purpose names: title artwork, introduction/ending illustration sets, dungeon scenery and creatures, creation artwork and story framing. Unidentified collections and individual illustrations retain neutral numbered labels; no guessed scene assignments are presented. The gallery preserves every original slot, including empty slots. Thumbnails use nearest-neighbor previews with a checkerboard for transparency. Tile tooltips reuse the map editor’s tile names. Slot and magnification selections survive navigation and Undo; magnification does not change exported dimensions. Story and creation editors link to related artwork collections where their engine use is verified. Damaged collections show recoverable errors.

Cutscenes lists the Blackthorn, Shrine, Codex and Final rescue scenes separately. Each route opens its matching scene and includes its message source in the tooltip. Character creation lives in Game Rules, with its backgrounds, questions and sequence in one editor. When scene data is absent, standalone message/creation-text routes remain available. Introduction, ending and virtue text remain dedicated destinations.

Game Rules groups prices and stock; entrances are edited on the map. Shop/service dialogue lives in NPCs under Shops & Services. The developer overlay coverage report is not exposed in the Workshop. Game Rules also groups starting party/world state and Look descriptions; starting objects are edited through Select / Move on the world maps; the effective Underworld object list combines initial and override sources. Direct resource access remains available for diagnostics. Change markers aggregate related content at the destination level. Resource serializers and mod package formats are unchanged.

## Mod Overview and aggregate changes (phase 6)

Select **Mod Overview** to open the project dashboard. It normally shows only
**Changed content** and **Validation**. **Advanced** reveals Package contents and
Compatibility details, including read-only engine coverage and unavailable NPC
data. Save, naming, export and test commands remain in the toolbar/menus rather
than being duplicated on the dashboard. It distinguishes changed content destinations from whole resources included in the package, explains Save/Export/Test and fresh-game behavior, and provides direct editor links. The package list also exposes changes that cannot be assigned to a named destination, including removed NPCs and unknown bytes. Explicit overrides of inherited defaults appear even when their bytes match the base.

**Run validation** checks the current project, engine decoder round trip and installed package conflicts without opening a modal dialog or launching the game. Results distinguish errors, warnings and information; double-click a source to open its editor or compatibility details. Export and Test retain their existing validation gates. Returning to the dashboard refreshes live changes and resets validation to “Not checked”; a saved project can still contain changes against the original game.

Location markers aggregate terrain on every floor, assigned conversations, schedules and signs. NPC markers compare individual schedule/appearance records and conversation identity; shared dialogue marks each user. Scene markers compare their own maps, message entries and creation sequence. Combat markers compare their own 352-byte encounter block. Categories show the number of changed visible destinations. Unrecognized or damaged layouts fall back to an explicitly labelled resource-level comparison; package contents remain authoritative. Undo, Redo and restoring original values update markers from live data. The dashboard and comparisons do not modify resources.

The toolbar keeps navigation, Save Project, Undo/Redo, Export, Validate and Test. **Project** contains New/Open/Save, naming and package import/export; **Tools** contains game-folder, validation and test configuration. Moved commands retain their shortcuts and draft guards. No file format, serializer or engine loading behavior changes.

Workshop opens new and reopened projects on Britannia, centered on Lord British’s Castle using the engine’s entrance defaults or the project’s location-entrance override. Thumbnails are limited to Britannia and Underworld. Canvas instructional popups and the permanent interaction-hint row have been removed; tool controls retain their help. NPC inspector widths use native Qt control and scrollbar measurements to fit platform fonts.

## Alternate tilesets

Tilesets contains DOS, Amiga, Apple II, Grayscale, and the Sharp X68000 terrain
and actor sheets. Edit individual tiles with the pixel canvas, import/export a
tile, or import/export a whole sheet. Choose color supports the native RGB palette;
Undo, project saving and package export include the edits. Built-in alternate
sheets are embedded in Workshop, so locating extra artwork files is unnecessary.

Source layouts remain unchanged: DOS/Amiga/Grayscale use 16×16 cells, Apple II
uses 14×16, and each Sharp sheet uses 32×32 cells. Tile positions retain their
original engine identities. Alternate sheets require opaque PNG pixels and use
the renderer's existing background colors: black, or gray for Sharp actors.
They support up to 240 colors; Sharp terrain and actors share that limit.

Exported mods carry complete edited PNG sheets independently of external artwork
files. Impera gives mounted mod sheets priority over bundled sheets. Select the
corresponding tileset in Engine Options to use the edited artwork; the package
does not change the player's tileset preference. Older Impera builds without
alternate-PNG package support reject these packages. Original game files and
bundled artwork remain untouched.

## Shop and service dialogue

In **NPCs → Shops & Services**, choose a service and then its greeting, refusal,
farewell, item description or service response. These are shared templates, not
individual NPC conversations. Shared variants remain one response; the compact
scope label and its tooltip identify that shared use. The NPC filter offers top-level locations and **Shops & Services**; selecting
Shops & Services shows only the shared service templates.

Edit one response in the normal application font. The right pane inserts shop,
keeper, customer, time, price, quantity and item values as inline labels. Original
compressed words appear as ordinary words and retain their byte encoding when
unchanged. Copy/paste within the editor preserves substitution identities.
There is no game-font preview, sample-values form, compression-code selector,
Apply button, or manual extended-storage switch. Shop transaction and branching
behavior remain controlled by the engine.

Valid edits save after a short typing pause, on response changes, and before
project actions, with normal project Undo/Redo. Oversized fixed entries use the
existing extended-text mod storage automatically. Invalid drafts stay in the
editor with a concise error and block navigation/export until corrected. Original
resources, shared offsets, token spacing, and unrelated narrative editors retain
their existing behavior. Changed markers identify the affected services.

Signs and starting objects are selected and edited in the map's right pane, with
no duplicate entries in normal navigation. Look descriptions belong to Game Rules.
If required maps or world-object companions are missing, direct resource recovery
remains accessible through Mod Overview's Advanced compatibility details.

On world maps, Gold and Food are direct **Add…** choices. Moonstone placement
selects one of the eight buried stones; use **Positions** to move it or make it
carried. Party starts and location entrances are also edited graphically there.
Game Rules links to the party start and moonstones on their maps.

See [the placement audit](workshop-placement-audit.md) for Add menu coverage,
starting settings and remaining native-format limits.
