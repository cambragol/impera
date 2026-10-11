# Workshop content navigation

Phases 1 and 2 of the overarching UI redesign replace file-first navigation with a persistent content navigator.

Choose World, Characters, Story & Scenes, Artwork & Fonts, Game Rules, New Game, or Mod Overview in the workspace selector. Search filters the current workspace by content name or category. World lists settlements once; selecting a settlement opens its main floor. Other floors remain in the editor's floor selector. Combat encounters use numbered labels where their purpose has not been verified.

The navigator stays available beside every editor. Content (Ctrl+L) focuses its search without replacing the editor or discarding drafts. Back and Forward (Alt+Left / Alt+Right) navigate independently of Undo and Redo. Validated drafts are committed before changing destinations; invalid drafts keep the current editor open. Returning through history restores editor selections and map views.

Semantic destination IDs resolve to the existing Project resources and map pages. There is no second editable data store, and serializers and package formats are unchanged. Filenames remain available in source tooltips. Asterisks indicate changes against original data, including explicit overrides, rather than unsaved project changes.

The existing standalone map navigation remains available to standalone map workspaces; the main application hides it in favor of the shared navigator. Existing editors, contextual tabs, map-to-dialogue links, dungeon-room links and read-only compatibility information remain accessible.

## Remaining phases

3. Unify location modes: terrain, schedules, signs, encounters and starting objects with a shared location context.
4. Build a cross-bank character index with unified schedules and conversations, shared-dialogue provenance and unassigned conversations. Until then Characters provides human-readable conversation and schedule collections.
5. Refine artwork names and previews and consolidate story, rules and new-game routes.
6. Add aggregate content change tracking, a dedicated Mod Overview dashboard and simplified project toolbar. Until then Mod Overview contains read-only compatibility details, and change markers follow the underlying resource scope.
