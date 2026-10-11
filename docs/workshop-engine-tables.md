# Shops, prices and location entrances

Workshop's **Engine tables** workspace edits verified game concepts using named
items, merchants and locations. Select a table, select an entry, change its value
and press **Apply**. **Inherit compiled default** removes that entry's override.
All changes participate in normal project undo/redo and package export. Originals
are never rewritten. The preview explains pricing, bundle sizes, stock termination
or entrance coordinates before export.

| Table | Meaning / constraints |
| --- | --- |
| Equipment prices | Base prices for all 48 equipment entries; buying and resale both use them. Original intelligence adjustment and rounding remain. 0–10,000 gold. |
| Reagent prices | Eight reagents in each of five shops; zero hides that reagent from that shop. 0–255 gold before adjustment. |
| Reagent bundle sizes | Quantity purchased in each shop, 0–99. Edit alongside its price. |
| Equipment shop stock | Nine shops with eight stock positions each. Choose a named equipment item or End of list. Later entries after an end marker are hidden. The final terminator is read-only. |
| Guild supply bundle prices | Three guild suppliers: bundles of three keys, four gems or five torches. 0–10,000 gold. The unused fourth column and unassigned fourth merchant row are read-only. |
| Horse prices | Three stables' base prices; original purchasing rules remain. 0–10,000 gold. The unassigned fourth row is read-only. |
| Location entrance columns / rows | Forty named location entrance coordinates, 0–255. Enter, exit and dungeon lookup use these coordinates. Terrain must also be painted in Maps; changing an entrance does not move its terrain. |

## Runtime and compatibility

These values ordinarily come from native compiled tables, not raw `DATA.OVL`
bytes. Workshop exports only explicit overrides in a mod-only **RULES.TBL**
companion. Omitted entries inherit Impera's compiled defaults. Removing the last
override removes the companion from the package. The companion uses `IRULES01`:
an eight-byte magic, a little-endian 16-bit count, then sorted five-byte records
(table ID, 16-bit entry index, 16-bit value). Stable IDs 1–8 match the table order
above. The shared Workshop/engine validator rejects invalid bounds, references,
reserved columns, duplicate entries and malformed data.

Packages containing this companion use **IMOD0003** and require an Impera build
with engine-table support. Version 3 can also contain narrative overrides and
ordinary resources. Existing version 1 and 2 packages remain supported. Older
engines reject version 3 explicitly rather than silently ignoring its changes.
Package rejection is atomic; conflicts use the existing whole-resource policy.
Removing a package restores compiled defaults. Existing world state in saves is
preserved; use an isolated Test Mod session for safe experimentation.

## Coverage inventory

The second workspace tab lists compiled symbols, encoded strings, consumer
references and unresolved spans. Its source is the generated
[coverage inventory](workshop-overlay-inventory.json), derived from `vars.c` and
runtime references. Rebuild it with `python scripts/generate-engine-tables.py`;
`--check` verifies that generated defaults, labels and inventory are current.
No original game files are needed for generation.

The eight editable tables' DOS spans are checked against original game data in
optional compatibility tests. The Britannia chunk index already has a supported
runtime data path. Other offsets are candidates, **not verified editable spans**:
text addresses can belong to other DOS overlays, native pointer tables do not
have DOS pointer layouts, and some declarations represent runtime state rather
than persistent data. Unverified bytes remain explicitly unresolved even if a
candidate symbol overlaps them. Other editions require separate verification.

The inventory does not imply every game rule is editable. Compiled algorithms,
remaining unverified tables, DOS driver layouts and translated/native strings
are not changed by arbitrary overlay edits. Workshop explains this limitation
before export; it exposes no misleading "all data supported" toggle.
