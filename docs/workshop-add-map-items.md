# Adding map items

Use **Add…** in the map tool row, choose an item, and click its destination.
The new item is selected and its properties open on the right. NPCs and signs
place once; containers, hidden contents, creatures and vehicles repeat until
Escape, right-click, another tool, or leaving the map cancels placement.
Arrow keys and Enter also place items. Previewing or cancelling changes nothing.

Use **Select / Move** to select an existing item or drag it. Overlapping kinds
have an item chooser. **Delete** or the trash button removes the selected item;
Undo restores the entire edit. Painting terrain does not create NPCs or erase
attached contents. Project saving and package export retain the entities.

## Available items

| Item | Maps | Properties and behavior |
| --- | --- | --- |
| NPC | Settlements | Name, appearance, new or shared conversation, schedule destinations, start hours and behavior. All destinations initially equal the placement cell. |
| Monster | Settlements | Uses an NPC slot with native attack behavior and no conversation. |
| Monster | Britannia / Underworld | A native roaming world creature. Species can be changed in the object inspector. |
| Monster | Dungeon combat rooms | A native monster placement with an editable species. |
| Monster | Outdoor combat templates | A starting position for encounter-generated enemies; the encounter determines species and count. Adding a position does not increase that count. |
| Sign | Britannia / settlements | Creates both a visible sign and text. Appearance can be sign, gravestone or cross. Dragging moves text and the visible tile together. |
| Container | Worlds, settlements, dungeon levels, combat maps | Chest with explicit item contents, locks, traps and refill settings. Starts empty; add items in the right pane. |
| Hidden contents | Same maps as containers | Searchable contents without changing the terrain, including trees, furniture and other hiding places. |
| Horse, skiff, frigate, magic carpet | Britannia / Underworld | Native usable objects. Frigates start with 99 hull condition. Place ships on suitable water. |
| Trigger | Dungeon combat rooms | Stepping on it changes up to two destination tiles. Edit destinations and replacement tile in the right pane; preview is optional. |

Dungeon feature painting continues to supply ladders, fountains, pits, fields,
walls and room doors. Dungeon levels have Add… for containers and hidden contents;
creatures are placed in the linked combat room.

## Compatibility and capacity

Native NPC and world-object identities are existing resource/slot identities;
custom contents use persistent namespace/ID identities. No extra engine entity
schema is needed for the native placements. Custom contents retain package v4
and its existing runtime/save behavior.

NPC creation reserves slot 0 for the party and uses only a slot with no appearance.
There are 31 usable slots per settlement. It never overwrites an existing NPC.
New conversations use an unused number from 1–127, checking both the dialogue
collection and references from every settlement in that bank. Native table,
1024-byte conversation and signed-offset limits remain enforced. Creation also
clears that NPC's killed bit in the fresh-game starting state. Existing saves can
retain their own killed flags; test new NPCs with a fresh game.

World placement reserves party slot 0 and, in the Underworld, quest slots 28–31
used for the amulet and shards. Object overrides affect new games. Combat rooms
retain 16 placement slots and eight trigger slots; party starts cannot be deleted.
Outdoor combat positions are available only where a coordinate is disabled.
Signs retain native size/floor constraints and the engine-generated Yew poster
cannot be replaced. Contents retain their existing 256-definition capacity.

Capacity or validation failures leave the project untouched. Creating an NPC and
conversation, creating a sign and tile, deleting an object and attached contents,
and dragging a sign are each one undoable transaction. Shared dialogue survives
NPC deletion. Removing/moving a freestanding sign infers its exposed ground from
adjacent grass, wood, brick or stone floor; Undo restores the exact previous tiles.

## Additional tile audit

The audit added **vehicles** and **combat triggers** to the original NPC / monster /
sign / container / hidden-content list. These need records beyond terrain artwork.

* Ordinary doors, portcullises, fields, furniture and stairs already work as terrain;
  duplicating them in Add… would create two ways to paint the same thing.
* Barrels, trunks, desks, dressers, trees and similar objects can carry **Hidden
  contents**. Painting their artwork alone never invents an inventory. These are
  Search targets; the chest Container option uses Open.
* Graves and crosses use Sign's appearance selector, rather than separate Add entries.
* Shopkeepers need shop definitions as well as an NPC. The Shops editor changes
  existing shops; arbitrary new shops/services require a future runtime extension.
* Entrances and portals need destination/quest state, not merely a tile. Existing
  entrances have table controls; Add… does not fabricate a destination.
* Moonstones and quest artifacts have special identities and engine-managed
  spawns. They are not offered as generic copies that could duplicate quest state.
* Loose pickup piles and arbitrary new service/portal definitions are candidates
  for a future extension. Hidden contents can supply ordinary items now, with
  Search revealing the pickup; it does not claim to be a pre-revealed pile.

Validation covers native slot reservation, missing capacity, duplicate targets,
shared dialogue deletion, starting killed bits, tile/text pairing, combat metadata,
package round trips, cancellation, repeated placement and UI undo. Native
Windows/macOS execution remains a CI check.
