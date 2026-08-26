# WoW 3.3.5a Client Memory Profile

This profile targets only the exact build-12340 executable identified in the
README. Absolute virtual addresses are appropriate for this file because its
PE relocation data is stripped and its preferred image base is `0x00400000`.
The reader DLL checks the PE headers before accessing these addresses.

## Statically verified for this executable

The following were traced through the supplied `Wow.exe` disassembly rather
than accepted only from third-party offset lists.

| Data | Address/layout | Result |
| --- | --- | --- |
| Character name | inline string at `0x00C79D18` | Corrected; the old expression evaluated to the wrong address. |
| Internal map name | inline buffer at `0x00CE06D0`, capacity `0x104` | Corrected from pointer-string reading. |
| Zone/sub-zone | pointers at `0x00BD0788` / `0x00BD0784` | Existing pointer reads retained. |
| Object manager | connection at `0x00C79CE0`, manager at `+0x2ED0`, first object at manager `+0xAC` | Existing chain retained. |
| Object GUID | object `+0x30`/`+0x34` | Existing 64-bit GUID read retained. |
| Party members | four inline GUIDs at `0x00BD1948`, stride 8; leader GUID at `0x00BD1968` | Iteration corrected from five slots to four. |
| Raid | count at `0x00BEB608`; pointer array at `0x00BEB568`; GUID at entry `+0` | Existing layout retained; count and index bounds added. |
| World frame/camera | world-frame pointer at `0x00B7436C`; active-camera pointer at frame `+0x7E20` | Contradictory legacy camera chain removed. |
| Map/state | map ID `0x00AB63BC`; in-world byte `0x00BD0792`; loaded DWORD `0x00BEBA40` | Reads aligned to observed storage. |
| Interaction GUIDs | mouseover `0x00BD07A0`; last target `0x00BD07B8` | Legacy aliases corrected. |
| Combo points | byte at `0x00BD084D` | Legacy alias corrected. |
| Power type | byte 3 of `UNIT_FIELD_BYTES_0`, descriptor offset `0x5F` | Corrected from the inconsistent hardcoded `0x47`. |

## Information still required from a live client

These areas cannot be considered reliable from the current static evidence.
For each capture, record the client state and the expected value visible in the
stock UI so the memory result can be compared against ground truth.

| Priority | Area | What is needed |
| --- | --- | --- |
| High | Player-name cache | Confirm the structure rooted at `0x00C5D938`, bucket/mask/node layout, and the role/signature of the lookup routine at `0x0067D770`. Capture local, party, raid, and unrelated visible-player GUID/name pairs. The current manual `nameStore + 8` hash walk is provisional. |
| High | Combat log | Capture several nodes from manager `0x00ADB974` and cursor `0x00CA1394` for known melee damage, spell damage/heal, aura apply/remove, miss, death, and environmental events. Confirm event discriminant, payload union sizes, amount fields, flags, and ownership/lifetime before relying on the current generic node layout. |
| High | Aura tables | For player, target, party, and raid units, capture unit base plus the regions at `+0xC50` through `+0xDD4` while known spell IDs are active. Confirm inline versus heap-table mode, entry stride `0x18`, spell ID at `+0x8`, count limits, and buff/debuff metadata. |
| High | Names for creatures/pets | Confirm object `+0x960` template pointer and name pointer offsets with several known NPCs, pets, and summons. The current `+0x18`/`+0x0` fallback is provisional. |
| Medium | Unit/object traversal | Capture the live manager and a short object list to confirm next-object link `+0x3C`, object type `+0x14`, descriptor pointer `+0x8`, and termination/sentinel behavior across login, zoning, and logout. |
| Medium | Camera fields | Compare camera struct `+0x30`/`+0x34` against known yaw/pitch changes. The pointer chain is verified; these two scalar field offsets still need runtime confirmation. |
| Medium | Party fallback stats | Validate the party-member cache base/stride and health/max-health offsets while a member is in range, out of range, dead, offline, and zoning. |
| Medium | Quest reader | Compare every active-quest entry, title, objective count/progress, completion state, and log-slot bounds with the stock quest log. |
| Medium | Casting/channel timing | In addition to spell IDs, locate start/end times, interruptibility, delay, and target GUID needed for usable cast bars. |
| Medium | Cooldowns/range | Confirm the internal cooldown and range function signatures, calling conventions, owner thread requirements, and return semantics before calling them. Addresses alone are insufficient. |

For internal functions, each binding needs more than an address: calling
convention, exact parameter and return types, required `this` pointer, valid
game-thread context, re-entrancy constraints, and object ownership/lifetime.
Calling an address without those details is substantially riskier than reading
a validated data structure.

## Information needed for a full default-UI replacement

The current transport exposes a diagnostic subset, not enough state to replace
the complete stock UI. A practical next profile needs stable sources for:

- action bars, spellbook, item/bag/equipment data, charges, cooldown timing,
  usability and range;
- complete unit frames: class, race, faction, level classification, reaction,
  threat, absorb/heal prediction, cast timing, raid roles and ready checks;
- chat channels/messages, loot, vendors, mail, trade, auction, social/guild,
  talents, glyphs, achievements, reputation, XP, currencies and quest details;
- textures/icons, localized strings, tooltips and secure input/action dispatch;
- input routing and an explicit, reversible method of hiding or replacing each
  stock UI surface.

Prefer public Lua/API data for high-level UI state when possible. Reserve raw
memory for values unavailable through that interface, and keep every absolute
address behind the exact-client guard.

## Suggested capture format

For reproducible validation, collect:

1. the inspector JSON for `Wow.exe`;
2. timestamped expected values from the stock UI or `/dump`/Lua output;
3. a narrowly scoped memory snapshot containing the relevant object and data
   pages (ordinary crash minidumps often omit these pages);
4. the absolute base addresses/pointers used to reach each captured structure;
5. login/zone/combat state and the action that caused the transition.

Do not include account credentials, session tokens, chat text, or unrelated
process memory in shared captures.
