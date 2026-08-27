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
| Player-name cache | cache object `0x00C5D938`, hash table `+0x8`; bucket base/mask at table `+0x1C`/`+0x24`; 12-byte buckets; node GUID/name at `+0x18`/`+0x20` | Recovered from `DbNameCache_GetInfoBlockById` (`0x0067D770`). The reader now follows the bucket-selected collision link instead of treating the table as a pointer array. |
| Creature names | template pointer at unit `+0x964`; name pointer at template `+0x5C` | Recovered from `CGUnit_C::GetUnitName` (`0x0072A000`); replaces the invalid `+0x960` / `+0x18` fallback. |
| Cast/channel timing | cast ID `+0xA5C`; spell/target/start/end at `+0xA6C`/`+0xA70`/`+0xA78`/`+0xA7C`; channel spell/start/end at `+0xA80`/`+0xA84`/`+0xA88` | Recovered from `UnitCastingInfo` (`0x00611DF0`) and `UnitChannelInfo` (`0x00612090`). Times are unsigned client milliseconds. |
| Aura entries | inline table `+0xC50`, heap count/pointer `+0xC54`/`+0xC58`, sentinel/count `+0xDD0`, stride `0x18`; caster/spell/flags/stacks/duration/expiry at entry `+0`/`+8`/`+0xC`/`+0xE`/`+0x10`/`+0x14` | Recovered from the stock aura accessors and `UnitAura` wrapper. |
| Camera angles | public yaw/pitch at active camera `+0x11C`/`+0x120`; smoothed view pitch/yaw at `+0x230`/`+0x260` | Recovered from `CommentatorGetCamera`, `FlipCameraYaw`, and `CGCamera` update/view functions. The former `+0x30`/`+0x34` reads were unrelated fields and have been removed. |
| Action-bar state | 144 slots; first action at `0x00C1E358`; count/usability/resource arrays at `0x00C1E118`/`0x00C1DED8`/`0x00C1DC98` | Recovered from `GetActionInfo`, `GetActionCount`, and `IsUsableAction`. Prefer the stock query wrappers for typed action semantics. |
| Quest-log UI cache | entries at `0x00C237B0`, stride `0x10`; cached/visible counts at `0x00C23AD0`/`0x00C23AE4`; selected quest ID at `0x00C23AD8` | Recovered from the stock quest-log query wrappers. Localized titles and objective text still come from the quest cache/query API. |
| Combat-log core | first entry `0x00ADB97C`, sentinel `0x00ADB978`, current/pending cursors `0x00CA1390`/`0x00CA1394`; next/time/type/source/destination/spell fields through node `+0x44` | Recovered from the stock combat-log wrappers. Event-specific payloads beginning at `+0x54` are a tagged union, not one generic layout. |

The broader stock query-function inventory is recorded in
[ghidra-ui-offsets.md](ghidra-ui-offsets.md). Those addresses are entry points,
not passive data: they require the game-owned Lua state and game-thread call
context.

## Information still required from a live client

These areas cannot be considered reliable from the current static evidence.
For each capture, record the client state and the expected value visible in the
stock UI so the memory result can be compared against ground truth.

| Priority | Area | What is needed |
| --- | --- | --- |
| High | Combat log payload union | Capture known melee damage, spell damage/heal, aura apply/remove, miss, death, and environmental entries. Map each event discriminant and `+0x54` payload tag to its payload size/fields and validate node ownership/recycling before enabling generic event reads. |
| High | Aura semantics | Compare raw flags, stack count, duration, and expiry against `UnitAura` for player, target, party, and raid units. The storage layout is static; filter bits and lifecycle behavior still need runtime ground truth. |
| High | Renamed pet names | Confirm the pet-name cache keyed by unit `+0xD0 -> +0x114` and revision `+0x118`. Ordinary creature names are statically resolved, but renamed controlled pets use `DbPetNameCache_GetInfoBlockById` (`0x0067EA30`). |
| Medium | Unit/object traversal | Capture the live manager and a short object list to confirm next-object link `+0x3C`, object type `+0x14`, descriptor pointer `+0x8`, and termination/sentinel behavior across login, zoning, and logout. |
| Medium | Party fallback stats | Validate the party-member cache base/stride and health/max-health offsets while a member is in range, out of range, dead, offline, and zoning. |
| Medium | Quest reader | Compare descriptor quest slots and the UI cache against every active quest, title, objective progress, completion state, collapsed header, watch state, and log-slot bound. Use the stock wrappers for localized text. |
| Medium | Casting/channel behavior | Validate delay/pushback and interruptibility against the stock cast bar. Spell, target, cast ID, and start/end offsets are statically resolved; the interruptible result is computed rather than stored beside them. |
| Medium | Query bridge | Establish a game-thread dispatcher around the game-owned Lua state, then validate cooldown/range/usability and other stock query wrapper return values. Addresses alone do not authorize calling them from the render thread. |

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
