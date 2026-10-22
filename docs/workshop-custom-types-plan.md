# Custom tiles, NPC types, and monster types

Status: phases 1–5 implemented on `feature/custom-tile-types`. Terrain/scenery
and independent NPC/monster type authoring are available. Phase 6 remains the
platform/release validation and broader parity pass.

Implemented: a shared validated catalog, independent extended IDs, terrain package v9 / actor package v10,
embedded art and animation, sparse map placements and save state, native property
consumers and interaction transitions, Workshop drawing/import/property controls,
and map painting/clipboard/contextual integration. Artwork is embedded in
`TYPES.OVR`; no separate `TYPEART.OVR` resource is needed. Vanilla map codecs and
packed save structs retain their original formats. Native interaction profiles
bridge the existing command handlers; mutable operations use typed setters so
custom identities are not written into byte-sized map data.

Use **Graphics → Custom tiles → Tiles**, or **New tile…** from an existing
tileset editor. Choose **Copy tile…** to start with a vanilla or custom tile's
resolved settings. The map inspector's **Edit tile…** opens the same definition.
Original tile overrides support Reset; additions support Delete after references
have been removed. World and Locations membership checkboxes are independent.

Interaction profiles reuse supported native interactions and their map contexts;
this does not introduce arbitrary executable scripts or additional quest systems.
Coordinates, sign text, contents, locks and traps remain in their existing map
panels. Dungeon feature codes are separate from encounter-room terrain.

## Authoring model

Workshop should let a mod add genuinely new definitions, without replacing an
existing Ultima V tile or borrowing an apparently unused actor slot. A modder can
start blank or copy an existing definition, draw/import its artwork, set its
properties, and immediately use it in the map editor. Export includes the
definitions, artwork, and placements in the ordinary `.imperamod` package.

Three things need independent identities:

* **Tile:** artwork, animation, description, movement/environment properties,
  interactions, and rendering properties.
* **NPC type:** appearance and movement, default job/description, combat
  properties and abilities. A placed NPC has its own name, dialogue and schedule.
* **Monster type:** appearance and movement, combat properties and abilities,
  encounter composition and death behavior. A placed monster has its own state.

An NPC's picture must not determine its combat statistics or dialogue identity.
Animation frames must not consume additional terrain or monster type IDs.
Changing an existing definition changes its instances; copying a definition
creates an independent type. Existing vanilla definitions remain available.
Allow explicit overrides of vanilla definitions with Reset to original, keeping
unchanged definitions inherited from the game. Merely inspecting one never
creates an override.

## What the engine actually does today

There is no complete DOS tile-property record that Workshop can simply expose.
Properties come from compiled tables, comparisons against specific tile IDs,
rendering code, commands, map records, and instance state. Editing artwork does
not edit those rules. `DATA.OVL` replacement is not a general solution: the native
port only interprets its supported overrides.

### Terrain and scenery behavior inventory

| Behavior | Current source | Required authoring representation |
| --- | --- | --- |
| Walking collision | `D_54d4` in `src/vars.c`; `ULTIMA_2bd4` in `src/2000.c` | Walkable, with the humanoid/chair exception represented separately |
| Water, sailing, flying, phasing and terrain-specific movement | `ULTIMA_2c4c_IsWalkableTile`, `D_54f4`, `D_5534`, `D_5544` | Terrain movement properties plus actor movement profile; directional boat/river access |
| Slow/very slow travel | `MAINOUT_03e0` in `src/mainout.c` | Normal/slow/very slow travel cost; preserve the original extra world turns and encounter processing, not just an animation delay |
| Swamp poison | `OUTSUBS_05fc_StepOnPoisonTile`, `TOWN_0f02` | Poison on entry/turn with applicable movement modes and original dexterity checks |
| Lava/fireplace damage | `OUTSUBS_05ee_StepOnBurningTile`, `TOWN_0f02`, `ULTIMA_2aa8` | Burning/damage effects, damage range, targets and timing; vanilla party damage rolls 1–8 per eligible member |
| Trapdoors and falling | `TOWN_0f02`, `OUTSUBS_0458_Falls` | Trapdoor/waterfall interaction and placement-specific destination; preserve flying-carpet exceptions |
| Rough seas and whirlpools | `src/mainout.c`, `src/outsubs.c` | Water depth and water hazards, separate from whether walking is allowed |
| Visibility blocking and adjacent window exceptions | `ULTIMA_5dfe` in `src/5000.c`; `Transparent` in `src/graphics/widescreen.c`; `D_6a86` | Sight: clear, blocking, or vanilla adjacent-window behavior; shared implementation for normal and expanded views |
| Projectile/spell passage | `D_6a14`; `ULTIMA_3f6e` in `src/3000.c`; callers in `src/comsubs.c` and `src/cast.c` | Projectile/spell passage, independent of walking and sight |
| Battle background selection | `ULTIMA_6150_Combat` in `src/6000.c` | Associated combat terrain/map profile, retaining ship and map-context overrides |
| Random encounters and camping context | `MAINOUT_0e4e` and nearby routines | Encounter habitat/profile and camping environment; custom tiles must not fall through to arbitrary numeric-ID comparisons |
| Doors, locked doors, opening and magical locking/unlocking | `src/cmds.c`, `src/cast.c`, `src/cast2.c`, `src/sjog.c` | Door interaction with named open/closed/locked variants; instance lock state where appropriate |
| Stairs, ladders and entrances | `src/town.c`, `src/mainout.c`, `src/cmds.c` | Direction/orientation and transition capability; target edited on the map |
| Sitting, sleeping, stocks and manacles | `ULTIMA_ResolveActorTile` in `src/5000.c` | Occupancy/pose profile and alternate appearance, independent of raw sprite IDs |
| Moving/rotating furniture and cannon | `src/cmds.c` | Move/rotate interaction, directional variants, vacated floor and cannon behavior |
| Destruction/transformation | `src/cast.c`, `src/cast2.c`, `src/town.c` | Named result types for burning/breaking/harvesting/unlocking; preserve original command restrictions |
| Search, chests and scenery containers | `src/lookobj.c`, `src/mod/contents*` | Interaction capability on the type; actual contents, lock, trap and refill on the placed instance |
| Signs, inscriptions and readable objects | `src/lookobj.c`, native sign resources | Read interaction and supported appearance; text belongs to the placed sign |
| Wells, fountains, telescope, clock | `src/lookobj.c` | Named interaction profiles; effects and clock behavior are not inferred from art |
| Mirrors and reflections | `src/town.c`, `ULTIMA_ResolveActorTile` | Mirror interaction/rendering profile and broken variant |
| Moongates, shrines, Codex and quest scenery | `src/4000.c`, `src/mainout.c`, `src/cast2.c`, `src/lookobj.c` | Explicit supported interaction profiles; quest identity and destination are not a generic “special” checkbox |
| Multi-cell descriptions and actor occlusion | `src/lookobj.c`, `src/5000.c` | Companion-cell/occlusion profile for composite vanilla scenery; bound and validate references |
| Crops, trees and other world transformations | `src/town.c`, `src/outsubs.c` | Harvest/change-state result and reset policy, with per-instance state |
| Ambient effects | `ULTIMA_4102_AudioPlayAmbientSfx` in `src/4000.c` | Clock, waterfall, fountain and musician ambient profiles |

The code has context-sensitive rules. World and location swamp poison do not
even use exactly the same random-range bounds. A field actor in combat is not
the same thing as a terrain square with a poison flag. Preserve vanilla context
rules in imported presets; do not flatten them into an inaccurate shared flag.

The table above identifies behavior families, not a claim that every hardcoded
branch has already been converted. During implementation, maintain a coverage
checklist of every tile-ID/table consumer and test its replacement. Include
numeric comparisons and bit masks, not only `TILE_MAP_*` constants.

### Movement profiles

The current dispatch has eleven profiles, plus an invalid value:

| Profile | Current rules |
| --- | --- |
| Walking | Collision bitset; selected humanoids can occupy chairs |
| Sea creature | Water and river tiles |
| Flying | Water/rivers or normally walkable ground; not unrestricted passage through walls |
| Horse | Walking rules, excluding swamp and lava |
| Phasing | Non-water under the existing sea-tile test |
| Skiff | Water/rivers, direction-sensitive river/bridge restrictions |
| Frigate | Deep and medium water only |
| Swamp only | Rot worm habitat |
| Grass only | Corpser habitat |
| Deep water only | Whirlpool habitat |
| Sand only | Sand trap habitat |

Use recognizable choices in the UI. Preserve these exact presets and allow
custom terrain access rules; do not describe flying as “ignore collision.”
World/Locations picker membership never changes movement permissions.

### Rendering and animation inventory

* DOS artwork consists of 256 terrain pictures and 256 actor pictures, logically
  16×16. Alternative tilesets retain native detail through the renderer's 32×32
  samples. New art must work in both normal and fullscreen presentation.
* `src/4000.c` uses fixed terrain animation groups and actor animation programs
  in `D_1b18`. Those programs include pauses/random choices, not merely four
  interchangeable frames. `src/graphics/tileset.c` also scrolls water artwork;
  the original graphics backend has procedural effects.
* Sprite cutout eligibility, backdrop inference, overlap/outline behavior and
  native PNG alpha are handled across `grap_buf.c`, `tileset.c`, `5000.c` and
  `widescreen.c`. There must be one definition used by all these paths.
* PNG alpha and sight transparency are entirely different properties.
* Masonry is a darkness-fade barrier, **not** collision or sight blocking. The
  audited vanilla list is in [masonry-tiles.md](masonry-tiles.md).
* Some actor poses contain their scenery; they are intentionally opaque. A new
  sprite must not acquire black outlines merely because it is actor artwork.

### NPC/monster data and behavior inventory

Vanilla defines 48 combat types. `EnemyStats` contains strength, dexterity,
intelligence, armor, attack damage, maximum HP, maximum encounter count, and a
treasure value. Additional tables supply attack range (`D_159c`), projectile
appearance (`D_15cc`), companion encounter type (`D_16d4`), singular/job names
(`D_1856`), encounter names (`D_18b6`) and movement/animation profiles.

The treasure byte feeds the original loot/chest logic; it is not a gold count.
XP is also derived from maximum HP in the current damage/death routine. Initial
combat dexterity has randomization and drives turn timing. Preserve those rules
for vanilla presets; expose overrides under clear labels only with runtime support.

| Existing flag | Audited meaning / proposed UI |
| --- | --- |
| `0x8000` | Magical attack; suppressed by the existing negation rules |
| `0x4000` | No active flag consumer found; preserve imported value, no invented ability |
| `0x2000` | Teleport |
| `0x1000` | Vanish on death |
| `0x0800` | Disappear/reappear during combat |
| `0x0400` | Summon a daemon; generalize summon target to a type reference |
| `0x0200` | Poison contribution; currently combined with `0x0004` by the damage handler |
| `0x0100` | No active flag consumer found; preserve imported value |
| `0x0080` | Use strength rather than dexterity for the default attack-stat query |
| `0x0040` | Possess/charm |
| `0x0020` | Undead; conditional half damage and undead spell interactions |
| `0x0010` | Divide after surviving damage |
| `0x0008` | Immune to ordinary damage in the native damage routine |
| `0x0004` | Poison contribution; do not label as “ranged poison” without a separate verified rule |
| `0x0002` | Steal food |
| `0x0001` | Leave no corpse |

Evidence: `src/macros.h`, `COMBAT_014e`, `COMBAT_13e2`, `COMBAT_1574`,
`COMBAT_194a`, and `COMSUBS_00f4`. Comments about uncertain original flags are
not sufficient evidence for a user-facing control.

There are also explicit type-index rules that must become capabilities or
well-defined legacy profiles:

* Mimic disguise/reveal and special attack scheduling.
* Reaper immobility and strength-based attack behavior.
* Gazer sleep on hit.
* Gargoyle death becomes rocks; gazer death can create an insect swarm.
* Corpser concealment/drag-under and associated victim state.
* Daemon removal after a successful possession.
* Shadowlord visibility, escape, quest-object and encounter handling.
* Slime/gargoyle division preserving the correct resulting custom type.
* Summoning, encounter companions, corpses/chests, and replacement actors.
* Creature-specific movement, visibility and idle animation.

These are spread through `src/combat.c`, `src/comsubs.c`, `src/6000.c`,
`src/npc.c`, `src/4000.c`, `src/town.c`, and overworld spawning. In particular,
merely expanding `D_13bc_enemyStats` would not create independent new monsters.

## Workshop UI

### Graphics → Tilesets

Keep the existing alternative tileset editing. Add a **New tile…** action and
make a tile definition the common selection, with artwork variants underneath
it. A custom tile appears once in the catalog, not once per animation frame.

Layout:

* Left: searchable tile grid, All/World/Locations filter, and New tile action.
* Center: pixel canvas and compact frame strip; add/import/delete/reorder frames,
  play/stop and frame duration. Pencil, fill, eyedropper and PNG import
  reuse the existing graphics editor where possible.
* Right: name and Look description, followed by Artwork, Movement & Effects,
  Interactions, and Appearance sections. Show controls relevant to the selected
  interaction, using ordinary names instead of bitfields or file offsets.

**New tile** asks for a name and optionally an existing tile to copy. Simple
presets such as Ground, Wall, Water and Decoration initialize complete settings;
they do not lock the user into a category. Copying snapshots the resolved settings
instead of creating a fragile inheritance chain.

The first visible properties should be:

* Name, Look description, and **World / Locations** checkboxes. Either or both
  can be checked; neither keeps it out of those filters while retaining it in All.
* Draw as Ground or Sprite. For Sprite, select the underlying ground policy
  (explicit tile or existing neighbor inference) and alpha/opaque rendering.
* Walkable; other movement modes in an expandable section; travel speed.
* Blocks sight; blocks projectiles; masonry fade barrier.
* On-entry/turn effects, including damage and poison, with condition/target fields.
* Interaction, default state, and relevant result/variant tiles.

“Ground” controls composition/floor behavior; it does not silently mean walkable.
Masonry, PNG alpha, sight and movement remain independent. Context-sensitive
presets show their resolved settings; there is no mysterious “acts like tile 74.”

PNG import must:

* Accept a single frame or explicitly sliced sheet, with a preview before applying.
* Support logical 16×16 and detailed 32×32 frames; reject mismatched dimensions
  rather than silently distort them. Preserve existing legacy alternative imports.
* Preserve PNG alpha, including opaque black; alpha art has no synthesized black
  outline. Newly authored PNGs should not require a transparency color.
* Offer artwork variants for DOS, Apple II, Grayscale, Amiga, Sharp, and Custom.
  One authored image is sufficient: missing alternatives use that tile's default
  art. Do not borrow unrelated art from the same numeric slot in another sheet.
* Keep imported pixels embedded in the project/package; no absolute-path dependency.

For new animation, support an ordered sequence, per-frame duration, and looping.
Existing procedural/idle animation presets must keep their native behavior when
copied. Custom animation affects appearance only unless an explicit state change
is authored. Collision cannot oscillate because an animation frame changes.

### Graphics → Custom tiles

Add a type-library destination to the existing NPC workspace, distinct from its
dialogue and Shops & Services filters. Include NPC types and Monster types in a
searchable list with an appearance thumbnail. The type editor uses the same
artwork/animation controls, with sections for:

* Identity: singular name, encounter plural, default job/Look description.
* Appearance and movement profile.
* Combat: all eight original stats, attack range/appearance, attack-stat source.
* Abilities: verified poison, sleep, theft, charm, teleport, invisibility, summoning,
  division, undead/invulnerability and special movement/death capabilities.
* Encounters: maximum group size and companion type; explicit spawn eligibility.
* Death: corpse/vanish/transform/spawn/loot rules.

NPC and monster types share the underlying actor definition. Their different
starting presets should not duplicate all the implementation. Names in a type
definition are defaults; an NPC instance's personal name and dialogue are separate.
Creating a “villager” type must not create a second NPC named “villager” on a map.
Types without dialogue must not be added to the dialogue editor's NPC list.

Expose verified abilities as controls, never a hex editor. Imported unknown bits
round-trip invisibly. Keep genuinely story-specific legacy quest behavior bound
to the original quest actors unless the modder explicitly chooses a supported
quest profile; copying Lord British's appearance must not copy his quest identity.

### Map integration

* New terrain/scenery tiles appear in the existing tile picker and paint normally.
  Vanilla World/Locations membership remains derived from map usage; custom
  definitions use their explicit checkboxes. The lists may overlap.
* Add… → NPC / Monster selects from vanilla and custom types with thumbnails.
  Include New type… and Edit type… shortcuts without duplicating the type editor.
* Select/Move opens the existing contextual inspector. Show a compact Type field
  and Edit type action. Name, dialogue, schedule, allegiance/current state and
  placement belong to the selected instance, not its shared type.
* Signs and containers keep their existing contextual panels. An arbitrary new
  scenery tile can have container capability without requiring a chest picture.
* Doors, stairs, entrances and portals edit destinations/states graphically.
  The type defines how an interaction works; the map instance defines where it goes.
* Painting, selecting, dragging, filling, copying, pasting, deleting, highlighting
  changes and undo all preserve full type identity. Multi-resource operations
  (for example, type plus new dialogue plus placement) are atomic.
* Keep brush, tool, filter, tile-panel width and viewport state across maps.

## Runtime and package design

### Identity and storage

The present map buffers, map clipboard, actor records and some combat indices
are bytes. Vanilla monster indexing is often `(shape - 0x40) / 4`. All 256 terrain
values and 256 actor pictures have existing meanings, including sentinel values.
There is no safe “unused tile range” for the requested feature.

Use stable definitions identified on disk by **library UUID + local numeric ID**,
with separate terrain/actor reference kinds. Assign dense integer handles at load
time for runtime speed. Vanilla references resolve through a built-in namespace.
Save/package files store stable references, never dense handles or display names.
Copying/importing a library retains identity; creating an independent type assigns
a new identity. Duplicate identities with different definitions are conflicts.

Preserve original DOS codecs and packed structs as import/export boundaries.
Do not widen `ActorFmt` or `S_6608_Map` in place: code reads/writes their original
fixed byte lengths. Introduce authoritative native map/actor state with expanded
handles and explicit conversion. Refactor mutable `ULTIMA_4402_GetTileAddr` users
to typed get/set operations; rendering-only overlays would lose changes made by
opening doors, spells, harvesting and combat.

Keep the existing per-map actor population limits initially; adding type
definitions and raising the number of simultaneous actors are separate changes.
Show the actual placement capacity in the existing map UI. Type count must not
be limited by the original 48 combat rows or by four adjacent artwork frames.

The implemented versioned resources are:

| Resource | Purpose |
| --- | --- |
| `TYPES.OVR` | Library identity, typed definitions, names, properties, embedded RGBA animation frames and references |
| `TYPEMAP.OVR` | Sparse custom terrain cells and actor instance-slot references keyed by stable map/page identity |

The wire layouts use a shared C validator in Impera and Workshop. Keep data-only packaging and existing size/allocation limits;
check decoded pixel budgets as well as encoded bytes. Do not add one package
resource per frame: the current package entry limit is 128.

Use a new minimum package version for extended types. Older engines must reject
it with the existing update diagnostic. Existing packages retain their minimum
version. Cross-resource references, map bounds, frame slices, dimensions,
durations, effect ranges and interaction targets validate before mounting.

The current loader gives the first package claiming a resource exclusive ownership.
Initially retain that documented rule for the new resources and reject conflicting
libraries as whole packages. UUIDs prevent accidental identity reuse; they do not
imply that arbitrary independently exported type libraries can already merge.
If independent library composition is included later, it needs an explicit
transactional namespace merge, not silent last-wins replacement.

### Shared rules

Introduce common tile/actor definition accessors backed by compiled vanilla
defaults plus validated custom definitions. Both engine and Workshop consume
the same definitions and validation. Refactor original behavior consumers into
these accessors in small, parity-tested steps.

Keep behavior profiles for verified context-sensitive vanilla logic. For custom
types, store actual editable properties/capabilities, not an uneditable behavior
alias. Omitted properties resolve to documented defaults, not arbitrary tile IDs.
Interaction result references form an explicit state graph. Validate inheritance
and description-companion cycles; allow legitimate door open/close state cycles.

Handle actor type through movement, combat initialization, stat lookup, target
selection, projectiles, divide/summon/companion spawning, animation, name printing,
death and cleanup. Never reconstruct its identity from an animation picture.

### Saves and initialization

Add versioned native extension state alongside current save data, containing
stable actor/type references, custom map mutations and interaction state. Integrate
it with both working saves and modern slot staging/rollback. Persist encounter
custom types if encounters are saved by the existing save contract; otherwise
respect the existing encounter restoration boundary.

Fresh-game initialization must install custom placements before the initial save
and first map frame, including a start inside Iolo's hut. Leaving/re-entering a
location must not reset custom state. Loading a slot validates required libraries
and references before replacing live state. Missing/incompatible definitions
produce an actionable error, never silently reinterpret a custom ID as vanilla.
Original saves without extension state continue to work with the vanilla registry.

## Implementation phases and completion gates

### 1. Shared definitions and vanilla behavior catalog

* Implement the C definition schema, typed references, validators and compiled
  vanilla defaults. Workshop gets a typed document layer and import/copy support.
* Record each hardcoded consumer above and map it to a named property/profile.
* Verify every vanilla movement combination (256 actor shapes × 256 terrain
  values), visibility/projectile properties, all 48 combat rows, known flag
  consumers, encounter companions, rendering traits and animation profiles.
* Keep uncertain unused bits opaque; add fixtures for context-dependent exceptions.

Gate: using the shared catalog reproduces vanilla properties; no custom type is
advertised as playable yet.

### 2. Package, expanded references, placements and persistence

* Add the extension resources and package capability/version checks, project
  persistence, collision/reference validation and deletion usage tracking.
* Implement expanded native terrain/actor identity without changing DOS codecs.
* Convert map mutation and actor lifecycle operations; add save extension state
  and transactional slot integration.

Gate: multiple custom types beyond vanilla ID ranges survive project export/import,
game start, leaving/re-entering a location and save/load without ID truncation.

### 3. Artwork, animation and tile-definition UI

* Add New tile, PNG/frame import and drawing, variants, animation strip/preview.
* Add property sections and explicit World/Locations membership.
* Render custom artwork in engine and Workshop, including alpha, opaque black,
  floor composition, fullscreen, darkness and masonry.
* Integrate terrain painting/clipboard/undo with expanded references.

Gate: a newly drawn animated tile can be painted and played with the authored
collision, travel cost, poison/damage and sight/projectile settings. Changing
tilesets has a deterministic artwork fallback. No misleading inactive controls.

### 4. Interactions and contextual map editing

* Implement supported interaction profiles and type/state transformations.
* Wire containers, signs, doors, transitions, destruction, occupancy, mirrors,
  ambient profiles and battle/encounter environment to common rules.
* Keep placement-specific text, contents and destinations in contextual panels.

Gate: custom doors, containers and readable scenery work with keyboard and mouse,
survive save/load, and preserve terrain/content identity when moved or transformed.
Complete the hardcoded-consumer checklist; don't claim full vanilla coverage while
unconverted interaction branches still key on old IDs.

### 5. NPC and monster type authoring

* Add the type library, shared appearance editor, stats/abilities, encounters and
  death rules. Connect Add… and existing contextual inspectors.
* Convert all combat/name/movement/animation consumers to independent type lookup.
* Support all verified special behaviors, with explicit targets for transformed,
  summoned, divided and companion actors. Preserve story-specific vanilla profiles.

Gate: two new types using the same artwork have different stats/abilities; two
different appearances can share behavior without sharing identity. Summoning and
division create the authored custom type. New NPC dialogue/schedules work from
the first frame at a fresh-game start and survive revisits.

### 6. Parity, end-to-end coverage and release integration

* Run original-data parity checks for movement, effects, sight, projectiles,
  interaction transformations, encounter selection and every vanilla actor type.
* Test the complete authoring → package → native runtime → save/load path on
  world, location, combat and dungeon-room maps. Dungeon feature bytes remain
  their own format; custom encounter art must not be written into dungeon nibbles.
* Test malformed images/resources, excessive allocations, dangling references,
  incompatible saves, duplicate libraries and atomic rollback on failure.
* Run Workshop UI regressions for editing/import, undo/redo, stable selection,
  type deletion with references, filtering and persistent map-editor state.
* Run native and packaged smoke tests on Windows, Linux and both macOS targets.
  Include custom-type package smoke coverage without distributing original assets.

Gate: all authorable properties have tested runtime consumers; every required
extension ships in the combined Impera/Workshop archives. Documentation accurately
states type limits, per-map instance limits, compatibility and package conflicts.

## Phase 4/5 implementation notes

Graphics → Custom tiles contains **NPC types** and **Monster types**. Both reuse the same
palette-limited, multi-tileset artwork editor as terrain. Type controls expose
all eight original combat statistics, movement, attack range/projectile style,
verified abilities, death/corpse behavior, and explicit transformation, summon,
division and encounter companion targets. Unknown original flag bits are kept
without invented controls. Creature descriptions in this library are plural
combat names; individual NPC descriptions, jobs, conversations and schedules
remain in their existing NPC/map editors.

The original behavior profile preserves special native rules (for example,
reapers, gazers, corpsers, mimics and story actors). It is separate from authored
artwork and combat statistics; it does not create a new quest scripting language.
Explicit transformation targets apply on defeat. Summoning and division use the
authored type target; a divided creature keeps the source's current HP, as in
vanilla. Encounter size and companion types configure the original group rules.
Specific combat-room spawn slots can also be assigned custom types.

Add… offers authored types; location and combat inspectors can change them or
open **Edit type…**. Starting world objects can change creature type too. Type
assignments are separate from movement/schedule coordinates and never occupy
DOS terrain IDs. Moving an instance keeps its type; deleting it clears its slot
reference. World slots reused by native spawns cannot inherit a removed custom
type. Native combat uses an extended in-memory type ID, while DOS actor records
and party save layouts remain unchanged.

Original instance limits remain: 31 location NPC slots, 31 surface world object
slots (27 in the Underworld), 16 room/ordinary encounter spawn positions, and 32
runtime combat entities including party members. There are up to 1024 authored
terrain/actor definitions across independent IDs 512–65535. Saves persist stable
actor slot references; native combat itself remains transient as before.
