# Workshop terrain tile catalog audit

Audited all 256 DOS terrain images against `TILES.16`, the original `LOOK2.DAT`
descriptions and engine interactions. Actor appearances are separate from this
terrain catalog. Original descriptions sometimes describe a logical tile whose
stored artwork is an animation/rendering fragment, so artwork and engine use were
checked together rather than treating either source as a complete taxonomy.

## Rules

* **Overworld:** decoded cells in Britannia or Underworld, from original or edited
  maps, plus animation frames of used tiles. No unused chunk storage or assumed
  location icons. This filter may overlap a base category.
* **Ground:** terrain, natural vegetation, roads, rivers and flooring.
* **Buildings:** architectural components: roofs, walls, crenellations, railings,
  windows, doors, bridges, stairs, ladders and fences.
* **Objects:** furniture, equipment, ornaments, torture devices, guardians,
  moongates, signs and visible object animations.
* **Other:** miniature location/entrance icons, effects and rendering fragments.
  Location icons appear in Overworld only if actually used by a world map.

The engine redirects Look on e0–e2 to an adjacent object; these are companion
pieces of multi-tile objects, not ordinary ground despite placeholder desert
descriptions. They are classified as Other.

Every terrain tile has exactly one base category. Favorites and Recent filters
and the Favorite button have been removed.

## Corrected misidentifications

| IDs (hex) | Identification | Category |
| --- | --- | --- |
| 10–1b, 39–3f | World location/entrance icons and multi-tile castle icons | Other |
| 27–28 | Roof segments, not carpets | Buildings |
| 41–42 | Codex and mast, not a book/counter meal | Objects |
| 46 | Pillar, not boulder | Objects |
| 4c | Pile of rocks, not a wall | Ground |
| 58–59 | Anvil and telescope, not skeletons | Objects |
| 5e–5f | Guardians of the Codex, not stone arches | Objects |
| 80–83 | Pendulum frames, not forge tiles | Objects |
| 84–85 | Stocks and manacles, not fence/web | Objects |
| 88 | Cannonballs, not rubble | Objects |
| 8b, 8d, 8e | Torture rack, harpsichord, guillotine | Objects |
| a2–a3 | Hitching post and stack of logs, not ground/bench | Objects |
| a9, aa | Pitcher and carpet | Objects |
| b3, bd, be, bf | Roasting spit, street lamp, candelabrum, stove | Objects |
| ca–cb | Wooden fence, not brick/grass flooring | Buildings |
| c0–c3, cc–cf | Rendering fragments, not ordinary ground | Other |
| dc | Moongate | Objects |
| df, e0–e2 | Collapsed entrance icon and multi-tile object extension pieces | Other |
| e8–eb, ec–ef | Hourglass and Britannia banner frames | Objects |

The NPC appearance at actor shape 96 is labeled **person in stocks**. Removing
the harpsichord from recognized floor inference also prevents moving/deleting a
sign from copying harpsichord artwork into the vacated cell.

## Complete inventory

All IDs below are hexadecimal terrain IDs; repeated labels distinguish frames
or component pieces by ID. No original game artwork is distributed with this document.

### Ground (63 tiles)

| ID | Artwork |
| --- | --- |
| `01` | Deep water |
| `02` | Water |
| `03` | Shoals |
| `04` | Swamp |
| `05` | Grass |
| `06` | Brush |
| `07` | Desert |
| `08` | Brush |
| `09` | Forest |
| `0a` | Tropical forest |
| `0b` | Hills |
| `0c` | Mountains |
| `0d` | High mountains |
| `0e` | Foothills and grass |
| `0f` | Foothills and forest |
| `1e` | Desert vegetation |
| `1f` | Desert vegetation |
| `20` | Road |
| `21` | Road edge |
| `22` | Road corner |
| `23` | Road corner |
| `24` | Road corner |
| `25` | Road corner |
| `26` | Road junction |
| `2c` | Crops picked |
| `2d` | Crops |
| `2f` | Cactus |
| `30` | Grass edge |
| `31` | Grass edge |
| `32` | Road and grass |
| `33` | Road and grass |
| `34` | Riverbank |
| `35` | Riverbank |
| `36` | Riverbank |
| `37` | Riverbank |
| `40` | Wood floor |
| `44` | Brick floor |
| `45` | Stone floor |
| `4c` | Pile of rocks |
| `60` | River |
| `61` | River |
| `62` | River |
| `63` | River |
| `64` | River |
| `65` | River |
| `66` | River |
| `67` | River |
| `68` | River |
| `69` | River |
| `6c` | River |
| `6d` | River |
| `6e` | River |
| `6f` | River |
| `8f` | Lava |
| `d4` | Waterfall |
| `d5` | Waterfall animation |
| `d6` | Waterfall animation |
| `d7` | Waterfall animation |
| `dd` | Desert animation |
| `e4` | Water |
| `e5` | Water edge |
| `e6` | Water edge |
| `e7` | Water |

### Buildings (42 tiles)

| ID | Artwork |
| --- | --- |
| `1d` | Bridge |
| `27` | Roof |
| `28` | Roof |
| `43` | Railing |
| `47` | Bridge |
| `48` | Bridge |
| `49` | Bridge |
| `4a` | Arrow slit |
| `4b` | Window |
| `4d` | Stone wall |
| `4e` | Hidden door |
| `4f` | Wall |
| `50` | Crenellation corner |
| `51` | Crenellation corner |
| `52` | Crenellation corner |
| `53` | Crenellation corner |
| `54` | Crenellation corner |
| `55` | Crenellation corner |
| `56` | Crenellation wall |
| `57` | Crenellation wall |
| `5a` | Window shelf |
| `6a` | Wooden bridge |
| `6b` | Wooden bridge |
| `86` | Metal grate |
| `87` | Archway |
| `8c` | Loose brick / trapdoor |
| `97` | Odd door |
| `98` | Odd door |
| `99` | Bars |
| `b8` | Door |
| `b9` | Door |
| `ba` | Door |
| `bb` | Door |
| `bc` | Fireplace |
| `c4` | Stair |
| `c5` | Stairs |
| `c6` | Stairs |
| `c7` | Stairs |
| `c8` | Ladder up |
| `c9` | Ladder down |
| `ca` | Wooden fence |
| `cb` | Wooden fence |

### Objects (99 tiles)

| ID | Artwork |
| --- | --- |
| `1c` | Banner |
| `29` | Crystal ball |
| `2a` | Emblem |
| `2b` | Hollow stump |
| `2e` | Fruit tree |
| `38` | Gargoyle statue |
| `41` | Codex |
| `42` | Mast |
| `46` | Pillar |
| `58` | Anvil |
| `59` | Telescope |
| `5b` | Potted plant |
| `5c` | Bookshelf |
| `5d` | Bookshelf |
| `5e` | Codex guardian |
| `5f` | Codex guardian |
| `80` | Pendulum |
| `81` | Pendulum |
| `82` | Pendulum |
| `83` | Pendulum |
| `84` | Stocks |
| `85` | Manacles |
| `88` | Cannonballs |
| `89` | Cross |
| `8a` | Gravestone |
| `8b` | Torture rack |
| `8d` | Harpsichord |
| `8e` | Guillotine |
| `90` | Chair |
| `91` | Chair |
| `92` | Chair |
| `93` | Chair |
| `94` | Table |
| `95` | Table |
| `96` | Table |
| `9a` | Table |
| `9b` | Table |
| `9c` | Table |
| `9d` | Mirror |
| `9e` | Mirror |
| `9f` | Mirror broken |
| `a0` | Signpost |
| `a1` | Well |
| `a2` | Hitching post |
| `a3` | Stack of logs |
| `a4` | Stone cross |
| `a5` | Desk |
| `a6` | Barrel |
| `a7` | Wine cask |
| `a8` | Vanity |
| `a9` | Pitcher |
| `aa` | Carpet |
| `ab` | Bed |
| `ac` | Bed |
| `ad` | Dresser |
| `ae` | End table |
| `af` | Trunk |
| `b0` | Wall torch |
| `b1` | Wall torch |
| `b2` | Brazier |
| `b3` | Roasting spit |
| `b4` | Cannon |
| `b5` | Cannon |
| `b6` | Cannon |
| `b7` | Cannon |
| `bd` | Street lamp |
| `be` | Candelabrum |
| `bf` | Stove |
| `d8` | Fountain |
| `d9` | Fountain animation |
| `da` | Fountain animation |
| `db` | Fountain animation |
| `dc` | Moongate |
| `de` | Sacred flame |
| `e3` | Flagpole |
| `e8` | Hourglass |
| `e9` | Hourglass |
| `ea` | Hourglass |
| `eb` | Hourglass |
| `ec` | Britannia banner |
| `ed` | Britannia banner |
| `ee` | Britannia banner |
| `ef` | Britannia banner |
| `f0` | Provisioner sign |
| `f1` | Government sign |
| `f2` | Armoury sign |
| `f3` | Healer sign |
| `f4` | Stable sign |
| `f5` | Guild sign |
| `f6` | Inn sign |
| `f7` | Apothecary sign |
| `f8` | Sign |
| `f9` | Shipwright sign |
| `fa` | Clock |
| `fb` | Clock animation |
| `fc` | Bellows |
| `fd` | Bellows |
| `fe` | Bellows |
| `ff` | Bellows |

### Other (52 tiles)

| ID | Artwork |
| --- | --- |
| `00` | Magic flash |
| `10` | Hut location |
| `11` | Codex shrine location |
| `12` | Keep location |
| `13` | Village location |
| `14` | Town location |
| `15` | Castle location |
| `16` | Cave entrance |
| `17` | Mine entrance |
| `18` | Dungeon entrance |
| `19` | Shrine location |
| `1a` | Ruined shrine location |
| `1b` | Lighthouse location |
| `39` | Blackthorn’s castle location |
| `3a` | Castle location tower |
| `3b` | Castle location battlements |
| `3c` | Castle location tower |
| `3d` | Castle location tower |
| `3e` | Lord British’s castle location |
| `3f` | Castle location tower |
| `70` | Dungeon corridor mask |
| `71` | Dungeon corridor mask |
| `72` | Dungeon corridor mask |
| `73` | Dungeon corridor mask |
| `74` | Dungeon corridor mask |
| `75` | Dungeon corridor mask |
| `76` | Dungeon corridor mask |
| `77` | Dungeon corridor mask |
| `78` | Dungeon corridor mask |
| `79` | Dungeon corridor mask |
| `7a` | Dungeon corridor mask |
| `7b` | Dungeon corridor mask |
| `7c` | Dungeon corridor mask |
| `7d` | Dungeon corridor mask |
| `7e` | Dungeon corridor mask |
| `7f` | Dungeon corridor mask |
| `c0` | Stone rendering fragment |
| `c1` | Stone rendering fragment |
| `c2` | Stone rendering fragment |
| `c3` | Rendering fragment |
| `cc` | Brick rendering fragment |
| `cd` | Brick rendering fragment |
| `ce` | Vegetation rendering fragment |
| `cf` | Vegetation rendering fragment |
| `d0` | Diagonal wall mask |
| `d1` | Diagonal wall mask |
| `d2` | Diagonal wall mask |
| `d3` | Diagonal wall mask |
| `df` | Collapsed dungeon entrance |
| `e0` | Multi-tile object base |
| `e1` | Multi-tile object left edge |
| `e2` | Multi-tile object right edge |

