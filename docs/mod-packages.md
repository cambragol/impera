# Impera mod packages

Impera automatically scans the **`Mods` folder beside its executable, AppImage or macOS app bundle**
at startup. Only files whose extension is `.imperamod` (case-insensitive) are
loaded, in deterministic bytewise filename order. There is no extraction or
execution of package content. Mounted resources are read-only startup snapshots;
read handles have the ordinary seek/read behavior expected by existing loaders.
Writable saves and engine settings never go through these overrides.

Release archives include that empty folder alongside both applications. For
AppImages, mods live beside the AppImage file, outside its read-only image. For
macOS, they live beside `Impera.app`, outside the signed bundle. The Ultima 5 data
folder remains the source of original resources used for package verification;
its old `Mods` folder is no longer scanned. Move existing packages to the new
application-side folder. Workshop Test Mod uses `U5D_MODS_DIR` to select its
isolated disposable session folder without loading or writing installed mods.

All entries are checked against the **unmodified original resources**, not a
previous mod's output. A malformed package, wrong base, or conflicting resource
rejects the **entire package**, with a diagnostic in `LOG.TXT`. This keeps related
resources such as `BRIT.DAT` / `DATA.OVL` together. The first valid package claiming
a resource wins; subsequent packages touching that resource are skipped even if
the byte ranges do not overlap. Rename/recombine packages or merge them in
Workshop rather than relying on silent last-wins behavior. Names/titles, loaded
counts and rejected packages are logged. Restart after changing the folder.

## Binary layout

All integers are unsigned little-endian 32-bit values; there is no padding.

| Header | Encoding |
| --- | --- |
| Magic/version | Eight ASCII bytes `IMOD0001` through `IMOD0007` |
| Title length | 1–256 UTF-8 bytes |
| Resource count | 1–128 |
| Title | Length bytes; embedded NUL forbidden |

Each resource follows in order:

| Resource field | Encoding |
| --- | --- |
| Filename length | 1–32 |
| Filename | Uppercase ASCII basename, no path components |
| Original size | Exact original resource byte count |
| Result size | 1–8 MiB |
| Original CRC-32 | Standard IEEE CRC-32 of original file bytes |
| Result CRC-32 | Standard IEEE CRC-32 of patched resource bytes |
| Patch count | Number of following spans |
| Each patch | Offset, length, then `length` payload bytes |

The result begins as the original bytes, truncated or zero-extended to result
size. Patches replace bytes in that buffer; they do not insert/shift bytes.
Spans must be nonempty, sorted, nonoverlapping and entirely within the result.
Zero patches are allowed (for example, a truncation). Trailing package bytes,
duplicate resource names and checksum mismatches are rejected.

CRC-32 detects accidental corruption and incompatible originals; it is not an
authentication/signature system. Packages contain game-resource edits, and valid
checksums do not by themselves guarantee that an arbitrary game's data is valid.
Workshop validates the supported resource formats before saving/exporting.

Version 5 adds native dialogue with explicit quotation marks and inherits all
version 4 container, version 3 table and version 2 narrative capabilities.
Workshop selects it when a modified TLK response uses the narration extension.
Older engines reject these packages with an update diagnostic, rather than
displaying extension bytes as dialogue. Packages without this feature retain
their existing minimum version.

An explicit-quotation response starts with the four non-NUL bytes `FC FD FC FB`,
after the two-byte question label prefix when present. This reserved prefix is
recognized only at response entry; the engine omits its automatic opening and
closing quotation marks. Workshop writes the actual quotes around speech and
writes narration unquoted. This supports leading/trailing narration, narration-only
responses, prompts, fallback answers and matching answers without changing their
control flow. The prefix is invisible in the authoring UI and Playtest. Name remains
an identity field, not a narration response. Original unmarked DOS dialogue uses
the original quoting behavior unchanged.

Allowed resources: uppercase basenames ending `.DAT`, `.NPC`, `.TLK`, `.16`,
`.CH`, `.HCS`, `.CBT`, plus `DATA.OVL`, `INIT.GAM`, `INIT.OOL`, `BRIT.OOL` and
`UNDER.OOL`. Drivers, DOS executable overlays other than the Britannia index,
settings, paths and saved-game files cannot be overridden. This is resource data
modding, not replacement executable code.

Limits: 32 MiB encoded package, 32 MiB combined decoded resources per package,
8 MiB per resource, 128 loaded packages, 64 MiB combined mounted resource data.
Failures leave original resources available and do not write to the installation.
Sparse packages omit unchanged resources and unchanged bytes where possible;
compressed graphics edits can necessarily change much of the compressed stream.

The shared parser/checksum implementation is `src/mod/package.c`, used by both
the native Workshop and Impera. `src/mod/runtime.c` mounts packages, and
`FILE_Open` applies overrides only to bare, read-only resource names. Explicit
paths and writable handles retain their existing behavior. The legacy DOS
Makefile has no SDL mod loader; this feature belongs to Impera's native port.

## World maps and starting objects

Impera now uses a replaced `DATA.OVL`'s Britannia chunk index at `0x3886` for both
normal gameplay and the expanded fullscreen viewport. Packages containing a map
or index edit are checked against the effective `BRIT.DAT`/index pair, including
earlier non-conflicting packages. Invalid chunk references or truncated data
reject the whole candidate package. Without an overlay override, the original
compiled index remains the default; other DOS overlay tables are not interpreted.
Prefer exporting the paired map/index edits together through Workshop.

Starting object overrides apply when creating a character or transferring one
from Ultima IV, before the normal modern initial save is serialized:

- Britannia starts empty unless a package explicitly replaces `BRIT.OOL`.
- Underworld starts from effective `INIT.OOL`, unless a package explicitly
  replaces `UNDER.OOL`, which takes precedence.
- Each object list must contain exactly 32 eight-byte records (256 bytes).
- A world start uses that world's template for its active object list too;
  settlement actors still come from the starting game state (`INIT.GAM`).

Existing runtime world files do not change these new-game templates. Existing
modern saves retain their saved party and object state; templates are not applied
again when loading a save or traveling. Normal world-save initialization for
first-time installation remains separate. Terrain resource replacements still
apply to the game normally; they are not restricted to new games.

Packages depending on this behavior require an engine build with Workshop Phase 1
support. The package encoding is unchanged, so older builds can decode a package
without implementing these effects. There is no minimum-engine-version field in
`IMOD0001`; version enforcement is reserved for a future format/schema extension.
Workshop's package preview reports these compatibility limits.

## Special scenes

`MISCMAPS.DAT` replacements preserve the original scene format. Both Workshop and
the native loader check the creation block (at `0x2c0`, bytecode at block `0x200`)
against the same instruction decoder: buffer capacity, actor references, tile
coordinates, positive waits, matched non-nested repeats and a reachable restart.
Unknown one-byte operations follow the original interpreter's no-op behavior;
Workshop preserves them and disables structural/parameter edits for that stream.

Extended Blackthorn, shrine, Codex and final-rescue text uses the existing
`TEXT.OVR` schema with new verified scene IDs. Overrides are resolved at the actual
message print, never by replacing a bulk message-bank read. Other entries still
use the original bank. These IDs require Phase 6 Impera: older narrative-enabled
builds reject unknown entries. DAT-only map/message edits retain package version 1.

## Companion versions

Version 2 permits the verified `TEXT.OVR` narrative companion. Version 3 also
permits [engine table overrides](workshop-engine-tables.md) in `RULES.TBL` and
continues to accept narrative and ordinary resource entries. Companions have an
empty original base and zero base CRC; the engine validates their schemas before
mounting the package. Ordinary resource checks remain unchanged in all versions.
Older engines reject unsupported versions explicitly. Current Impera retains
version 1 and 2 compatibility.

### Illustrated cutscene pages (version 6)

`CUTSCENE.OVR` replaces either or both illustrated page sequences without editing
original DAT files. It requires `IMOD0006`; it has an empty base, like the other
virtual companions. Schema `ICUT0001` is followed by little-endian u32 introduction
and ending counts. A zero count inherits that original sequence; an override has
1–128 pages. At least one sequence must be overridden.

Introduction records precede ending records. Each record contains a u32 layout
index (0–20 for the Introduction, 0–5 for the Ending), a u32 text length including
the terminator, and that many bytes of NUL-terminated ASCII. The limit is 2,000
bytes including the terminator. Embedded NULs, unsupported controls, characters
above `{`, invalid layouts, truncated records and trailing bytes reject the whole
package. Layouts reference the original artwork arrangement and text exclusions;
the editor and engine share proportional wrapping and positioning. The portal
layout uses its first newline to separate its upper and lower text regions.

A cutscene override takes precedence over the original sequence, including any
`TEXT.OVR` ending entries. An inherited Ending still honors `TEXT.OVR`. As with
other whole-resource companions, overlapping cutscene packages conflict rather
than silently overwriting one another. Original 21-scene Introduction and six-page
Ending playback, including their transitions, remain the fallback.

### Per-page cutscene placement (version 7)

`IMOD0007` adds `ICUT0002` while retaining all previous package and cutscene
schemas. Schema 2 keeps the sequence counts and page headers from schema 1, but
inserts twelve little-endian u32 values between each page's layout/text-length
header and its text:

1. Custom placement enabled (0 or 1).
2. Artwork anchor X.
3. Artwork anchor Y.
4. Text left edge outside the wrap band.
5. Text right edge outside the wrap band.
6. Text left edge inside the wrap band.
7. Text right edge inside the wrap band.
8. Wrap band top.
9. Wrap band bottom.
10. First text line X.
11. First text line Y.
12. Lower portal paragraph Y.

A disabled custom flag inherits the selected original layout. Artwork anchors
must lie within the 320 × 200 page. Text edges must form nonempty intervals within
0–320, the wrap band must be ordered within 0–200, text X within 0–319, and text
Y/lower paragraph Y within 0–191. All encoded coordinate values are bounded by
320, even on records whose custom flag is disabled. Invalid values reject the
entire package. An inactive wrap band can use top = bottom = 200.

Custom artwork placement translates the entire original illustration group,
including decorative lettering, portal art and secondary/revealed frames. Drawing
clips to the game page, and the reveal region is clamped before page transfers.
The same placement object and font-layout kernel drive Workshop previews and
Impera playback. Layout changes are per page; duplication copies them and reordering
moves them with their text. If no page needs custom placement, Workshop emits the
original schema 1 / package version 6 instead.
