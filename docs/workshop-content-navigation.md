# Workshop content navigation

Phases 1–5 of the overarching UI redesign replace file-first navigation with a persistent content navigator.

Choose World, Characters, Story & Scenes, Artwork & Fonts, Game Rules, New Game, or Mod Overview in the workspace selector. Search filters the current workspace by content name or category. World lists settlements once; selecting a settlement opens its main floor. Other floors remain in the editor's floor selector. Combat encounters use numbered labels where their purpose has not been verified.

The navigator stays available beside every editor. Content (Ctrl+L) focuses its search without replacing the editor or discarding drafts. Back and Forward (Alt+Left / Alt+Right) navigate independently of Undo and Redo. Validated drafts are committed before changing destinations; invalid drafts keep the current editor open. Returning through history restores editor selections and map views.

Semantic destination IDs resolve to the existing Project resources and map pages. There is no second editable data store, and serializers and package formats are unchanged. Filenames remain available in source tooltips. Asterisks indicate changes against original data, including explicit overrides, rather than unsaved project changes.

The existing standalone map navigation remains available to standalone map workspaces; the main application hides it in favor of the shared navigator. Existing editors, contextual tabs, map-to-dialogue links, dungeon-room links and read-only compatibility information remain accessible.

## Location modes (phase 3)

Location editing uses one map canvas, floor selector, zoom and camera across these modes:

- **Terrain:** paint, select, copy and paste terrain.
- **Characters / Schedules:** inspect settlement characters, move their destinations, set schedule-change hours and open their conversations. The schedule selector appears in this mode.
- **Signs:** edit sign text, add a sign record at the targeted cell, and inspect its location. Only records on the current location floor appear. This changes text records, not terrain; place an appropriate readable sign tile in Terrain mode too.
- **Starting Objects:** add and move fresh-game objects in Britannia or the Underworld. The selected world is fixed by the shared map; existing saves are unaffected. Adding uses the targeted map cell. Explicit overrides, inheritance and undo retain their existing semantics.
- **Encounter:** place party starts, monsters and triggers on combat maps, with read-only trigger-result previews.
- **Encounter Details:** the existing advanced metadata controls, fixed to the selected combat map. Unverified/padding values remain explicitly identified.

Modes appear only when applicable data is available. Settlement floors remain in the shared selector; signs have no second location/floor picker, and starting objects have no second world picker. Painting controls are hidden in non-terrain modes. Terrain shortcuts from Characters or Encounter return to Terrain; external text/object modes cannot accidentally paint.

Switching modes or floors commits valid pending sign text first. Invalid text keeps the current mode and floor open. Undo, navigation history and map-to-conversation return retain the active location mode. Selecting a sign or object does not move the camera automatically; use Show selected sign/object to center on it. Damaged companion data shows a recoverable layer error rather than closing the application.

The standalone text and starting-object collections remain accessible for project-wide editing. Dungeon level/room editing remains its specialized workspace.

## Characters (phase 4)

Characters now lists individual scheduled characters across all four conversation banks, grouped and filtered by top-level location. Search matches character names and locations. Character identity includes its bank, location and schedule slot; equal conversation numbers in different banks do not refer to the same conversation. Empty appearance slots are excluded. Unassigned conversations remain available in a separate group; they may still be referenced by special game logic.

Selecting a character opens **Conversation** and **Schedule** views without a second bank/NPC browser. The heading identifies its location. Provenance lists every scheduled character sharing the conversation and makes clear that conversation edits affect them all; schedule edits affect only the selected character. Characters without editable dialogue still have a Schedule view and an explanation of engine-controlled dialogue.

Schedules show four changes with start hour, destination coordinates, floor and behavior. Changes 2 and 4 share their destination and behavior, and editing either updates both. Existing unusual values remain editable within their byte ranges. Appearance and conversation-link controls are available; an appearance code of zero disables the schedule entry. Changing a conversation link resolves the character's new conversation without copying dialogue. **Show character on map** opens the first scheduled floor with the character selected. Map conversation and schedule links open the same character workspace.

Valid conversation drafts commit before switching view or navigating; invalid drafts prevent leaving. Undo and Back/Forward preserve character identity and the selected view. The existing conversation editor retains topics, questions, preview, simulation and validation. Compatibility routes for damaged or empty banks remain available. No resource or package format changes are introduced. Change markers still follow the underlying conversation/schedule resource scope; per-character aggregate tracking remains phase 6.

## Artwork, stories, rules and new games (phase 5)

Artwork collections use verified purpose names: title artwork, introduction/ending illustration sets, dungeon scenery and creatures, creation artwork and story framing. Unidentified collections and individual illustrations retain neutral numbered labels; no guessed scene assignments are presented. The gallery preserves every original slot, including empty slots. Thumbnails use nearest-neighbor previews with a checkerboard for transparency. Tile tooltips reuse the map editor’s tile names. Slot and magnification selections survive navigation and Undo; magnification does not change exported dimensions. Story and creation editors link to related artwork collections where their engine use is verified. Damaged collections show recoverable errors.

Story & Scenes lists the Blackthorn, Shrine, Codex and Final rescue scenes separately. Each route opens its matching scene and includes its message source in the tooltip. Character creation lives in New Game, with its backgrounds, questions and sequence in one editor. When scene data is absent, standalone message/creation-text routes remain available. Introduction, ending and virtue text remain dedicated destinations.

Game Rules groups prices, stock and location entrances with shop/service dialogue. Engine overlay coverage belongs to Mod Overview and opens its read-only coverage tab. New Game groups starting party/world state and world objects; the effective Underworld object list appears once, with both initial and override sources recorded. Direct resource access remains available for diagnostics. Companion change markers use resource scope; precise aggregate tracking is still phase 6. Resource serializers and mod package formats are unchanged.

## Remaining phases

6. Add aggregate content change tracking, a dedicated Mod Overview dashboard and simplified project toolbar. Until then Mod Overview contains read-only compatibility details, and change markers follow the underlying resource scope.
