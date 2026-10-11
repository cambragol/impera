# Workshop content navigation

Phases 1–6 of the overarching UI redesign replace file-first navigation with a persistent content navigator.

Choose World, NPCs, Story & Scenes, Artwork & Fonts, Game Rules, New Game, or Mod Overview in the workspace selector. Search filters the current workspace by content name or category. World lists settlements once; selecting a settlement opens its main floor. Other floors remain in the editor's floor selector. Combat encounters use numbered labels where their purpose has not been verified.

The navigator stays available beside every editor. Tools → Find Content (Ctrl+L) focuses its search without replacing the editor or discarding drafts. Back and Forward (Alt+Left / Alt+Right) navigate independently of Undo and Redo. Validated drafts are committed before changing destinations; invalid drafts keep the current editor open. Returning through history restores editor selections and map views.

Semantic destination IDs resolve to the existing Project resources and map pages. There is no second editable data store, and serializers and package formats are unchanged. Filenames remain available in source tooltips. Asterisks indicate changes against original data, including explicit overrides, rather than unsaved project changes.

The existing standalone map navigation remains available to standalone map workspaces; the main application hides it in favor of the shared navigator. Existing editors, contextual tabs, map-to-dialogue links, dungeon-room links and read-only compatibility information remain accessible.

## Location modes (phase 3)

Location editing uses one map canvas, floor selector, zoom and camera across these modes:

- **Terrain:** paint, select, copy and paste terrain.
- **NPCs / Schedules:** inspect settlement NPCs, move their destinations, set schedule-change hours and open their conversations. The schedule selector appears in this mode.
- **Signs:** edit sign text, add a sign record at the targeted cell, and inspect its location. Only records on the current location floor appear. This changes text records, not terrain; place an appropriate readable sign tile in Terrain mode too.
- **Starting Objects:** add and move fresh-game objects in Britannia or the Underworld. The selected world is fixed by the shared map; existing saves are unaffected. Adding uses the targeted map cell. Explicit overrides, inheritance and undo retain their existing semantics.
- **Encounter:** place party starts, monsters and triggers on combat maps, with read-only trigger-result previews.
- **Encounter Details:** the existing advanced metadata controls, fixed to the selected combat map. Unverified/padding values remain explicitly identified.

Modes appear only when applicable data is available. Settlement floors remain in the shared selector; signs have no second location/floor picker, and starting objects have no second world picker. Painting controls are hidden in non-terrain modes. Terrain shortcuts from NPCs or Encounter return to Terrain; external text/object modes cannot accidentally paint.

Switching modes or floors commits valid pending sign text first. Invalid text keeps the current mode and floor open. Undo, navigation history and map-to-conversation return retain the active location mode. Selecting a sign or object does not move the camera automatically; use Show selected sign/object to center on it. Damaged companion data shows a recoverable layer error rather than closing the application.

The standalone text and starting-object collections remain accessible for project-wide editing. Dungeon level/room editing remains its specialized workspace.

## NPCs (phase 4)

The NPCs workspace lists individual scheduled NPCs across all four conversation banks, grouped and filtered by top-level location. Search matches NPC names and locations. NPC identity includes its bank, location and schedule slot; equal conversation numbers in different banks do not refer to the same conversation. Empty appearance slots are excluded. Unassigned conversations remain available in a separate group; they may still be referenced by special game logic.

Selecting an NPC opens its conversation directly, with a compact NPC/location heading and **Show NPC on map**. Shared-dialogue assignments are available in the heading tooltip. The duplicate schedule tab and full-width dialogue-users button are removed. NPCs without editable dialogue are excluded from this browser; their schedules remain editable in the map’s NPCs / Schedules mode.

The Conversation view has one authoring surface with speech, protected inline controls and compact action cards. Question prompts, ordered answers and Otherwise responses share that surface. Preview, Playtest and validation open only when requested; see [the dialogue redesign](workshop-dialogue-redesign.md) for details.

Schedules belong to the map editor: drag NPCs to change destinations, choose the schedule change, and edit its start hour in the NPC inspector. Advanced schedule details retain behavior, appearance and conversation-link editing. Changes 2 and 4 still share their destination and behavior. **Show NPC on map** opens the first scheduled floor with the NPC selected.

Valid conversation drafts commit before switching view or navigating; invalid drafts prevent leaving. Undo and Back/Forward preserve NPC identity and the selected response. Navigation rebuilds preserve the list’s scroll position and expanded location groups, including when a pending draft commits on focus change. NPC navigation labels omit spoken introduction closing quotes without changing dialogue bytes. The existing conversation editor retains topics, questions, preview, simulation and validation. Compatibility routes for damaged or empty banks remain available. No resource or package format changes are introduced. Change markers identify the selected NPC’s schedule and conversation; shared conversation edits mark every NPC that uses it.

## Artwork, stories, rules and new games (phase 5)

Artwork collections use verified purpose names: title artwork, introduction/ending illustration sets, dungeon scenery and creatures, creation artwork and story framing. Unidentified collections and individual illustrations retain neutral numbered labels; no guessed scene assignments are presented. The gallery preserves every original slot, including empty slots. Thumbnails use nearest-neighbor previews with a checkerboard for transparency. Tile tooltips reuse the map editor’s tile names. Slot and magnification selections survive navigation and Undo; magnification does not change exported dimensions. Story and creation editors link to related artwork collections where their engine use is verified. Damaged collections show recoverable errors.

Story & Scenes lists the Blackthorn, Shrine, Codex and Final rescue scenes separately. Each route opens its matching scene and includes its message source in the tooltip. Character creation lives in New Game, with its backgrounds, questions and sequence in one editor. When scene data is absent, standalone message/creation-text routes remain available. Introduction, ending and virtue text remain dedicated destinations.

Game Rules groups prices, stock and location entrances with shop/service dialogue. Engine overlay coverage belongs to Mod Overview and opens its read-only coverage tab. New Game groups starting party/world state and world objects; the effective Underworld object list appears once, with both initial and override sources recorded. Direct resource access remains available for diagnostics. Change markers aggregate related content at the destination level. Resource serializers and mod package formats are unchanged.

## Mod Overview and aggregate changes (phase 6)

Select **Mod Overview** to open the project dashboard. It distinguishes changed content destinations from whole resources included in the package, explains Save/Export/Test and fresh-game behavior, and provides direct editor links. The package list also exposes changes that cannot be assigned to a named destination, including removed NPCs and unknown bytes. Explicit overrides of inherited defaults appear even when their bytes match the base.

**Run validation** checks the current project, engine decoder round trip and installed package conflicts without opening a modal dialog or launching the game. Results distinguish errors, warnings and information; double-click a source to open its editor or compatibility details. Export and Test retain their existing validation gates. Returning to the dashboard refreshes live changes and resets validation to “Not checked”; a saved project can still contain changes against the original game.

Location markers aggregate terrain on every floor, assigned conversations, schedules and signs. NPC markers compare individual schedule/appearance records and conversation identity; shared dialogue marks each user. Scene markers compare their own maps, message entries and creation sequence. Combat markers compare their own 352-byte encounter block. Categories show the number of changed visible destinations. Unrecognized or damaged layouts fall back to an explicitly labelled resource-level comparison; package contents remain authoritative. Undo, Redo and restoring original values update markers from live data. The dashboard and comparisons do not modify resources.

The toolbar keeps navigation, Save Project, Undo/Redo, Export, Validate and Test. **Project** contains New/Open/Save, naming and package import/export; **Tools** contains game-folder, validation and test configuration. Moved commands retain their shortcuts and draft guards. No file format, serializer or engine loading behavior changes.

Workshop opens new and reopened projects on Britannia, centered on Lord British’s Castle using the engine’s entrance defaults or the project’s location-entrance override. Thumbnails are limited to Britannia and Underworld. Canvas instructional popups and the permanent interaction-hint row have been removed; tool controls retain their help. NPC inspector widths use native Qt control and scrollbar measurements to fit platform fonts.
