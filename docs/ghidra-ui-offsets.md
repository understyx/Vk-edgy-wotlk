# Ghidra-derived UI query profile

This inventory targets only the build-12340 `Wow.exe` identified in the
README. It was recovered from the open Ghidra program `/Wow.exe` by resolving
named functions and decompiling representative callers. The constants live in
`WoWOffsets::UIQuery` and `WoWOffsets::UIInternal` in
`include/wowmemory/offsets.h`.

## How to interpret these addresses

The `Script_*` functions are the stock client's Lua callbacks. Their effective
callback contract is a game `lua_State*` argument and an integer result count.
They are useful because they already implement the client-specific object,
cache, localization, cooldown, range, and nil-result rules needed by a UI.

They are **not** passive getters. Call them only through a future dispatcher
that runs on the owning game thread with the game-owned Lua state. Do not call
them from the Vulkan presentation/render thread. Mutation and protected-action
callbacks are intentionally excluded from this query inventory.

## Core action and spell UI

| Query | Address |
| --- | ---: |
| `GetActionCount` / `IsUsableAction` / `GetActionBarPage` | `0x005A7D10` / `0x005A7E60` / `0x005A7FD0` |
| `HasAction` / `GetActionInfo` / `GetActionCooldown` | `0x005A8220` / `0x005A8F10` / `0x005A91C0` |
| `GetActionTexture` / `IsActionInRange` | `0x005A9B30` / `0x005A9D50` |
| `GetNumSpellTabs` / `GetSpellTabInfo` | `0x0053B5C0` / `0x0053BE70` |
| `GetSpellName` / `GetSpellLink` / `GetSpellInfo` | `0x005407F0` / `0x005408E0` / `0x00540A30` |
| `GetSpellTexture` / `GetSpellCount` / `GetSpellCooldown` | `0x00540D70` / `0x00540DF0` / `0x00540E80` |
| `IsUsableSpell` / `IsSpellInRange` | `0x00541680` / `0x00541C60` |

`GetActionCooldown` and `GetSpellCooldown` return start and duration in seconds
after converting the client's millisecond counters. Range queries return nil
when the action/spell has no applicable range test; this is distinct from
false/out-of-range.

## Unit frames, casts, auras, groups

| Surface | Stock query entry points |
| --- | --- |
| Identity | `UnitExists 0x0060C2A0`, `UnitGUID 0x0060E630`, `UnitName 0x0060E740` |
| Classification | `UnitFactionGroup 0x0060D0A0`, `UnitReaction 0x0060D280`, `UnitClassification 0x0060D970`, `UnitRace 0x0060FD40`, `UnitClass 0x0060FEC0`, `UnitLevel 0x0060F9E0` |
| Health/power/XP | `UnitHealth 0x0060EB60`, `UnitHealthMax 0x0060EC60`, `UnitPower 0x0060ED40`, `UnitPowerMax 0x0060EF40`, `UnitPowerType 0x0060F100`, `UnitXP 0x0060EA60`, `UnitXPMax 0x0060EAE0` |
| Cast bars | `UnitCastingInfo 0x00611DF0`, `UnitChannelInfo 0x00612090` |
| Auras/range | `UnitAura 0x00614D40`, `UnitInRange 0x00612F10` |
| Threat | `UnitThreatSituation 0x00613A60`, `UnitDetailedThreatSituation 0x00613B40` |
| Groups | `UnitGroupRolesAssigned 0x0060C810`, `GetReadyCheckTimeLeft 0x00572C80`, `GetReadyCheckStatus 0x00574180`, `GetRaidRosterInfo 0x00573690` |

The passive cast and aura layouts are separately recorded in
`WoWOffsets::UnitCast` and `WoWOffsets::Aura`. `UnitCastingInfo` shows that
interruptibility is computed from spell/game state rather than stored in the
adjacent cast timing fields.

## Quest log and items

| Surface | Stock query entry points |
| --- | --- |
| Quest list | `GetNumQuestLogEntries 0x005DF010`, `GetQuestLogTitle 0x005E5CC0` |
| Objectives/text | `GetNumQuestLeaderBoards 0x005E41A0`, `GetQuestLogLeaderBoard 0x005E5F60`, `GetQuestLogQuestText 0x005E0340`, `GetQuestLogCompletionText 0x005E06D0` |
| Quest item | `GetQuestLogSpecialItemInfo 0x005E52D0`, `GetQuestLogSpecialItemCooldown 0x005E53D0`, `IsQuestLogSpecialItemInRange 0x005E54C0` |
| Bags | `GetContainerNumSlots 0x005D74A0`, `GetContainerItemInfo 0x005D7A90`, `GetContainerItemLink 0x005D7C80`, `GetContainerItemCooldown 0x005D7D90` |
| Equipment | `GetInventoryItemCooldown 0x005E7E60`, `GetInventoryItemTexture 0x005E9BC0`, `GetInventoryItemCount 0x005E9E40`, `GetInventoryItemDurability 0x005EA170`, `GetInventoryItemLink 0x005EA270`, `GetInventoryItemID 0x005EA3E0` |
| Item cache | `GetItemInfo 0x00516C60`, `GetItemIcon 0x00517020`, `GetItemCount 0x0051C2E0`, `GetItemCooldown 0x00510FC0`, `IsItemInRange 0x0051C9C0` |

## Stock windows and character panels

| Surface | Representative read-only entry points |
| --- | --- |
| Loot | `GetNumLootItems 0x00588540`, `GetLootSlotInfo 0x00588570`, `GetLootSlotLink 0x005886D0` |
| Merchant | `GetMerchantNumItems 0x005841D0`, `GetMerchantItemInfo 0x00584E10` |
| Mail | `GetInboxNumItems 0x0056D6D0`, `GetInboxHeaderInfo 0x0056E520`, `GetInboxItem 0x00570F10` |
| Trade | `GetTradePlayerItemInfo 0x00587EB0`, `GetTradeTargetItemInfo 0x00587C60` |
| Auction | `GetNumAuctionItems 0x0059C1A0`, `GetAuctionItemInfo 0x0059D5E0`, `GetAuctionItemLink 0x0059C2D0` |
| Friends/guild | `GetNumFriends 0x006B4060`, `GetFriendInfo 0x006B4130`, `GetNumGuildMembers 0x005CA130`, `GetGuildRosterInfo 0x005CC9C0` |
| Talents/glyphs | `GetNumTalentTabs 0x005C5CC0`, `GetNumTalents 0x005C5D40`, `GetTalentInfo 0x005C7800`, `GetNumGlyphSockets 0x005B71E0`, `GetGlyphSocketInfo 0x005B7260` |
| Achievements | `GetCategoryList 0x005B1390`, `GetAchievementInfo 0x005B3FC0`, `GetAchievementCriteriaInfo 0x005B58B0` |
| Reputation/currency | `GetNumFactions 0x005CFF20`, `GetFactionInfo 0x005D1150`, `GetCurrencyListSize 0x005AFD10`, `GetCurrencyListInfo 0x005B0680`, `GetMoney 0x0060FBA0` |
| Chat configuration | `GetChatWindowInfo 0x004FBD90`, `GetChatWindowMessages 0x004FC0C0` |
| Cursor/focus | `GetCursorInfo 0x00515200`, `GetMouseFocus 0x00516BF0` |
| Localization/tooltips (internal, not Lua callbacks) | `FrameScript_GetText 0x00819D40`, `CSimpleUI::CreateTooltip 0x00621070` |

`GetChatWindowMessages` reports the message groups assigned to a chat window;
it is not a stored chat-history reader. A replacement chat frame must consume
the stock `CHAT_MSG_*` event stream. This build also has no stock
`GetActionCharges`, total-absorb, or incoming-heal query callback: action stack
counts come from `GetActionCount`, while absorb/heal prediction must be derived
from aura/combat events or an addon communication source.

## What still needs runtime work

Static analysis provides addresses and field access, but it cannot establish
safe cross-thread invocation or reproduce live cache lifetimes. Before this
inventory becomes the UI transport, add one game-thread query dispatcher and
validate nil/error behavior while logged out, loading, zoning, in combat, and
with uncached item/quest/name records. Protected action dispatch and hiding the
stock UI require a separate, explicit design and are not covered by these
read-only offsets.
