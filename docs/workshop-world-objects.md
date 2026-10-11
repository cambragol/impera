# Starting world objects

Open Britannia or Underworld in Maps and select **Starting world objects**, or
open its entry under Starting state in the Resource Library. Select an object in
the named list or map, drag it to a destination, or edit its column/row. Add object
creates a frigate in the first empty slot; choose another shape in its inspector.
All changes support Undo/Redo and are exported as mod overrides. Original files
and existing saves are never changed.

The list has 32 fixed records. Slot 0 is the active party, so the editor exposes
31 object slots and reports remaining capacity. Slot numbers and original fields
remain in tooltips/Advanced details. The object artwork uses the project's tiles.
Ships expose hull condition; unknown/type-specific properties remain read-only.
Deleting empties the shape/frame, preserving other bytes in that slot.

The view matches the engine's new-game precedence:

| World | Inherited state | Explicit override |
|---|---|---|
| Britannia | Empty list (normal start) | BRIT.OOL from the mod |
| Underworld | INIT.OOL (initial underworld objects) | UNDER.OOL from the mod |

The source folder's BRIT.OOL/UNDER.OOL can contain old play state and are not
silently inherited. Editing materializes the corresponding world override.
**Revert to inherited** removes it undoably. Explicit inclusion is stored in the
project and package even when an override is byte-identical to the base; this
allows an explicitly empty Underworld as well as a no-op template override.
INIT.OOL is available in Advanced inspection, but the visual editor presents
one effective Underworld rather than asking modders to resolve file precedence.

Starting state sets the party and initial location. If it starts directly on a
world map, Impera also initializes its resident objects from this same list.
Settlement NPCs remain part of INIT.GAM/schedules, not this layer. **Test Mod →
Start a fresh game** makes a private, empty runtime: select Create A New Character
and then Journey Onwards to exercise the templates. Normal saves are not reset.
World lists are serialized by the existing save/travel paths, retaining their
state after entering/leaving settlements, world travel and reload.

## Verified record fields

Eight bytes: base shape, current animation shape, x, y, floor, type-specific
property, property, property. Actor artwork is tile 256 + shape. Empty means base
shape zero; animation shapes are not a second object. Worlds are 256×256; surface
floor is 0 and Underworld is 255. `ActorFmt`, `OUTSUBS_0368_GetWorldSavefile`,
`ULTIMA_368e_FindActorTileAtPos` and `MAINOUT_0000` establish addressing/rendering.

For frigates (shape 0x20–0x27), byte 5 is hull (`MAINOUT_109e`, rough-seas damage,
`ULTIMA_3c9a_HoleUpCmd`). Initial ship purchases use 99. Byte 7 may represent
aboard skiffs (`MAINOUT_0d22_Entry`/sinking); it is kept read-only because that
field is also reused for monsters/combat. Byte 6's general meaning is unresolved.
Chests, treasure, moonstones and shards reuse byte 5 with different semantics
(`SJOG_112c_OpenChest`, `SJOG_1458_GetObject`, `OUTSUBS_0458_Falls`); these values
are preserved, not presented as hull or generic NPC flags. Unusual inherited
records receive diagnostics rather than automatic deletion.
