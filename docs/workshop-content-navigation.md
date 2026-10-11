# Workshop content navigation

Phases 1–3 of the overarching UI redesign replace file-first navigation with a persistent content navigator.

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

## Remaining phases

4. Build a cross-bank character index with unified schedules and conversations, shared-dialogue provenance and unassigned conversations. Until then Characters provides human-readable conversation and schedule collections.
5. Refine artwork names and previews and consolidate story, rules and new-game routes.
6. Add aggregate content change tracking, a dedicated Mod Overview dashboard and simplified project toolbar. Until then Mod Overview contains read-only compatibility details, and change markers follow the underlying resource scope.
