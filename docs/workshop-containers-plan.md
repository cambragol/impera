# Map contents and containers

Status: all five implementation phases are complete. Local Linux engine and Workshop
validation is recorded below; native Windows/macOS runs remain a CI check.

## What vanilla does

There are two different systems, both of which need a friendly map inspector.

* Ordinary actor chests generate treasure when opened. Their property byte
  combines treasure strength and a trap bit; it is not an inventory. Dungeon
  chests also generate treasure, partly according to depth.
* Fixed discoveries use 113 search-location entries in compiled engine data.
  Each entry identifies a map, floor, column, row, item shape and item value.
  Searching creates a collectible actor at that cell; Get subsequently awards
  it. The terrain supplies the scenery and some search wording.

Minoc's ground-floor tree at column 2, row 2 is search entry 14. Its shape is
keys (7) and its value is 133: the high bit means skull keys, and the remaining
value means five. Search checks the current day against the saved last Minoc
search day, then reveals five keys and updates that saved day. This entry is
exempt from the normal one-time discovery bits. The tree has no nested inventory.

Evidence: `SJOG_095c_SearchCmd`, `SJOG_0514`, and `SJOG_1458_GetObject` in
`src/sjog.c`; the six arrays starting at `D_3e78` in `src/vars.c`;
`D_57b2` and its save/load paths. Other exceptional discoveries have their own
conditions, including replacement when the party has no ordinary keys and
replacement of a particular quest item. They must not become generic one-time
loot through automatic conversion.

The Workshop now exposes these discoveries and explicit container contents in
the map inspector. RULES.TBL remains separate from the new contents schema.

## Scope and defaults

Add explicit contents to any searchable map cell, and to placed containers.
Cover settlements, Britannia, Underworld, dungeon cells and encounter templates.
The default is **Original game behavior**. Opening a panel never creates an
override. Modders can choose **Custom contents** or **Empty**; custom contents
replace the matching original discovery or chest loot, rather than adding a
second reward. Random treasure remains the original path unless overridden.

First release supports ordinary inventory items, gold, food, torches, gems,
ordinary keys and skull keys. Quest items require verified state effects before
being enabled. Do not expose an item solely because it has a sprite. Nested
containers and configurable random loot tables are later work.

## Package extension

Implemented logical resource: `CONTENTS.OVR`, with its own versioned, bounded schema
and shared engine/Workshop validator. It uses package version 4 (IMOD0004), so old engines reject unsupported mods.

Each definition has:

* Stable mod namespace and definition ID, independent of coordinates.
* Target: location, signed floor, cell and interaction (Search or Open). Actor
  targets additionally reference a stable original slot or mod-owned object ID;
  encounter targets identify the template, not a transient combat map ID.
* Optional original search-entry identity, allowing a moved override to suppress
  the old discovery without suppressing unrelated discoveries at that cell.
* Mode: custom or empty; absence inherits original behavior.
* Contents: typed item IDs and quantities, using an explicit skull-key item ID
  rather than an encoded quantity flag.
* Refill: once per game, once per game day, or original special condition when
  editing an inherited discovery. Arbitrary scripting is out of scope.
* Container properties: lock state, lock difficulty, trap state/difficulty and
  supported trap effect. Gate controls by verified object behavior. A tree can
  be searchable without becoming a locked chest.
* Optional opened/empty appearance; otherwise retain the terrain. Changing a
  picture alone must not grant contents or change collision behavior.

Definitions are sparse. Restore original removes the override. Explicit empty
records survive export. Validate bounds, item eligibility, quantities, target
identity, duplicate IDs and conflicting targets. Multiple mods replacing the
same target use documented deterministic load order and report the conflict;
contents are never merged implicitly. Reject corrupt resources before mount.

## Runtime and save behavior

Resolve definitions before the relevant vanilla Search/Open paths. Untouched
locations continue through the existing code. Keep hidden-door discovery,
moonstones, reagents, corpses and special quests outside accidental interception.
Locks integrate with Open, Jimmy and unlock magic; traps integrate with search,
disarm and opening. Do not treat the vanilla chest trap bit as an independent
lock flag. Establish the supported mechanics from each original handler first.

Search/Open reveals contents, and Get collects them, matching vanilla. Keep
unclaimed items associated with the definition even after leaving a location.
Use an extension-owned pending-item store so large contents lists cannot destroy
actors or exceed the original 32-slot actor list. Render/pick exposed items via
that store, with ordinary keyboard and mouse Get behavior. Claim updates must
be atomic: inventory limits, failed interaction or unavailable display slots
must not consume items or mark an unopened container as looted.

Persist opened/unlocked/disarmed state, remaining quantities and refill day in
a versioned, checksummed trailer in SAVED.GAM keyed by mod namespace and stable
definition ID. Keeping party and contents in one staged file avoids a partial
companion-file save; save slots retain the trailer.
Integrate it with the save lifecycle so a partial save cannot restore consumed
loot. New games clear it; saves without it receive documented initial state.
For overrides of inherited discoveries, import their existing vanilla consumed
bits/day where possible to avoid granting already-collected rewards again.
Unknown removed-mod records stay dormant; coordinate moves retain identity.
Refill cannot accumulate a second batch while the previous batch remains.
Encounter instances need separate state keys so repeat battles do not share one
container's state; this is a prerequisite to enabling encounter contents.

## Map editor interaction

Use the existing **Select / Move** tool and shared right-hand inspector. No new
top-level tab, persistent instructions or separate contents workspace.

* Show subtle editor markers for inherited discoveries and custom contents.
  An optional overlay toggle hides markers without disabling selection.
* Clicking the Minoc tree opens **Tree — Contents**, showing **Skull keys × 5**
  and **Refill: Every game day**, initially marked as original behavior.
* Clicking a chest opens **Chest**, with compact **Contents** and **Properties**
  sections. Contents use named items, quantity controls and small Add/Remove
  buttons. Properties show only applicable lock, trap and refill controls.
* Clicking a cell with no existing discovery offers **Add hidden contents** in
  the right pane. Do not assume every decorative tree or barrel contains loot.
* Overlapping NPC/sign/container selections use the existing disambiguation
  pattern. Terrain brushes always edit terrain; Select / Move edits entities.
* Dragging a discovery marker moves its search point without moving the tree.
  Dragging a standalone container moves its object and contents together.
  Moving scenery and its discovery together can be an explicit selection
  operation; never silently rewrite terrain during a contents edit.
* Repainting a cell with attached contents preserves the definition but reports
  a mismatched appearance when appropriate. Removing a placed container deletes
  its attached contents atomically; undo restores both as one action.
* Every edit participates in project undo/redo, location change markers,
  validation, save/load and package import/export. Selection/preview is read-only.

The panel prioritizes item names and state, not coordinates, record numbers,
encoded bytes or implementation notes. Existing position controls suffice for
precise placement. Tooltips explain individual controls when necessary.

## Implementation sequence

1. **Audit and inherited discovery model.** Decode all fixed discoveries into
   named records, verify each exceptional refill/quest condition, and display
   them through Select / Move. Include Minoc and its daily five skull keys as a
   regression fixture. Audit lock/trap handlers separately from chest loot.
2. **Schema and project support.** Implement the bounded shared codec, sparse
   overrides, stable identities, capability checks and mod-conflict resolution.
   Test original inheritance, explicit empty, movement, undo and roundtrips.
3. **Search contents and persistence.** Implement fixed contents, pending items,
   Get integration and save companion. Test once-only/daily loot, inventory
   capacity, travel, save/reload, old saves and simultaneous discoveries.
4. **Containers and contextual inspector.** Add Open/lock/trap handling and the
   right pane. Verify keyboard/mouse interaction, unlock magic, failed attempts,
   overlaps, drag cancellation, terrain independence and restore original.
5. **Dungeon and encounter support, release validation.** Add their target/state
   adapters only after instance lifetime and save behavior are tested. Run engine
   and Workshop integration suites and a packaged-mod smoke test on all release
   platforms. Confirm unmodded Minoc, random chests and special quests retain
   original behavior. Reject unsupported capabilities explicitly.

## Using the implemented editor

Choose **Select / Move**, then click a discovery marker or chest to open its
right-hand inspector. Choose **Custom contents**, use **Add** to choose named
inventory items, and adjust their quantities. **Empty** suppresses the original
reward; **Original game behavior** removes the override. Adding an item to an
empty cell or empty definition makes it custom. The map toolbar's **Contents
markers** checkbox controls the overlay.

**Search** reveals hidden contents; **Open** creates a container interaction.
Open containers offer independent lock/trap controls. Open/Search reveals the
items and **Get** collects them. Unclaimed excess remains available when the
party reaches its inventory limit. Hidden discoveries move independently of
terrain. Placed world chests and their definitions move/delete together.

Minoc's tree at (2, 2) shows five skull keys and daily refill in original mode.
Changing or moving it exports only its override. The old search site is
suppressed when that inherited discovery moves. Viewing a panel or marker
never modifies the project.

Dungeon levels use the same right pane through Select / Move; combat maps use
explicit template identities. Each battle gets fresh contents state. Leaving
combat discards that instance's unclaimed items, matching the encounter's
lifetime; the game does not offer mid-combat saves. World, settlement and dungeon
pending items persist across travel and saves. Daily refill never accumulates
another batch while items remain unclaimed.

## Compatibility and limits

* Up to 256 active definitions, 16 item entries per definition. Persistent
  state retains up to 256 identities, including dormant removed-mod records.
* Gold/food quantities are bounded at 9,999; other quantities at 99. Collection
  uses the game's actual inventory capacity and retains excess.
* Combat targets use the existing 16 outdoor / 112 dungeon templates. Workshop
  validates actual maps, floors, templates and placed-chest references.
* Packages are scanned in filename order. Disjoint contents packages coexist.
  A conflicting target, inherited discovery identity or namespace/ID rejects
  the entire later package; contents lists are never merged implicitly.
* Trap effect is the engine's chest damage. Arbitrary trap scripting, random
  loot tables, nested containers and optional appearance overrides are not
  implemented. Untouched random chests continue through the original handlers.
* Quest discoveries remain inherited unless deliberately replaced by ordinary
  contents or Empty. Quest state effects are not exposed as generic items.
* Save files without the trailer remain valid. New games clear extension state.
  Overrides import vanilla discovery consumption where applicable. Extended
  saves require this version of Impera and are not DOS-game save files.

## Validation

The shared codec tests cover invalid targets/quantities/flags, original Minoc
semantics, moved discovery suppression, conflicting packages, pending items,
refill and save corruption. Engine interaction tests exercise Search/Open/Get,
Jimmy, unlock magic, trap damage, inventory limits, a full original actor list,
dungeon appearance and independent encounters. Workshop tests cover sparse
package/project roundtrips, selection without edits, dungeon/combat target
routing, terrain-independent movement, atomic chest deletion and undo. A
Workshop-exported package can be exercised by the engine adapter using
`IMPERA_CONTENTS_TEST_PACKAGE`.

These tests run alongside the existing engine and Workshop regression suite.
Native Windows and macOS packaging/gameplay validation requires their CI runners;
this cloud environment can build and execute Linux only.

Local validation: all 28 engine/Workshop CTest checks passed with `U5_GAME_DIR`
pointing to the original game data. The Workshop suite passed AddressSanitizer;
contents codec and interaction tests passed AddressSanitizer/UndefinedBehaviorSanitizer.
The exported Workshop package was decoded and its configured eight skull keys
were revealed and collected through the engine adapter.
