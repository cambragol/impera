# Isolated dialogue game context (Playtest phase 4)

Playtest now defaults to the Project's current `INIT.GAM`, including all sixteen
party/recruit records, equipment, supplies, quest flags, clock and NPC history.
The selected NPC's edited schedule bank supplies its actual conversation link,
location, appearance and the other NPCs in that location. Schedule selection uses
native unsigned hour differences, including ties and the shared second/fourth
destination. Actor placement preserves the native property flags generated from
the engine's authoritative table.

Setup contains an NPC selector. Navigation from a particular NPC selects that
identity; a uniquely assigned conversation selects its only user. Shared and
unassigned conversations never silently choose an arbitrary NPC. Conversation-only
testing can still render dialogue, but recruitment, guards and payments require
the relevant NPC context. Errors appear outside the native text viewport.

`src/text/dialogue_game.c` implements the native action algorithms once for both
Impera and Workshop. The engine binds explicit existing globals; Workshop binds
private byte spans. Neither adapter assumes the engine globals are contiguous.

- Payments preserve insufficient-funds behavior and beggar rewards, including the
  turn threshold, last-gold bonus and karma cap.
- Inventory grants preserve the 48 equipment counters, food/gold limits, supply
  caps and ownership flags. Spell mixtures and unrelated quest bytes are retained.
- Recruitment retains the native three-byte name search through inactive records,
  complete 32-byte record swap, full-party key wait/refusal and unmatched-name
  failure. Successful recruitment applies native killed-history rules and despawn,
  preserving actor properties and the schedule's coordinates/times.
- Guard calls retain the native active-NPC iteration, pursuit/flee schedule changes,
  conversation reassignment, random draws and unchanged coordinates.
- Introduction history addresses the selected location/NPC bit, preserving all
  other met/killed flags.

Each execution owns its random state. The generator matches the supported
platform's native `rand`: glibc Linux, Windows CRT and Darwin. First meetings
reseed as Impera does; later guard draws continue that sequence. Portable tests
compare multiple seeds and 1,024 draws per seed against the platform library.
During Talk, frame updates disable world animation/wind/movement, leave the game
clock unchanged and restore the animation flag. Wait/Pause tick requests remain
native. Offscreen non-combat vitals rendering runs through the shared text renderer
to retain its wrap delays without drawing another panel in the conversation view.

The optional Actions panel reports the resulting party/supply state and changed
NPC schedules. Restart always creates a fresh copy. Worker results expose complete
private starting-state/NPC/schedule/actor snapshots for differential verification;
they are never applied back to Project, installed packages, original files or saves.

## Compatibility and validation

Legacy equipment operands 48–63 accessed beyond the engine's 48-byte array. Their
effects depended on the compiler's global layout. Both runtimes now stop on these
invalid operands, and Workshop validation reports an error. Valid original item
IDs retain native behavior; unused native supply operands remain no-ops.

The independent frozen dialogue reference still uses the original give/join code
and original town guard routines. The expanded corpus compares its full initial
and final game-state artifacts against the engine adapter and Workshop host,
alongside conversation pixels, glyph/scroll boundaries, source positions, input,
branches, frame requests and virtual time. Added cases cover recruitment success,
full-party refusal, unmatched recruits, guard reactions after met/first meetings,
beggar reward/threshold/insufficient funds, all valid inventory grants and caps.
Unit/UI tests cover edited starting resources, ambiguous identities, preservation
of unrelated bytes, RNG parity, simultaneous workers, restart and cancellation.

Rules packages are validated when constructing context. These dialogue actions
use native hardcoded caps and behavior; unrelated shop/combat rule overrides do
not alter them. Unsupported overlay spans are not assigned invented effects.
Full-game Test Mod remains necessary for movement, combat, special location-entry
events and later consequences. The surrounding frame/zoom presentation remains
phase 5. Linux differential validation is local; Windows and both macOS architectures
still require their CI builds, including the native-library RNG check.
