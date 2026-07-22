/**
 * @file offsets.h
 * @brief WoW WotLK 3.3.5a memory offsets (translated from Offsets.cs)
 *
 * All values are absolute virtual addresses within the WoW process.
 * Offsets marked "relative" are byte offsets within an object — they must
 * be added to the relevant base pointer obtained from another absolute address.
 *
 * Usage (absolute address):
 *   uint32_t id = *reinterpret_cast<uint32_t*>(WoWOffsets::mapID);
 *
 * Usage (relative offset, e.g. player health):
 *   uintptr_t base = *reinterpret_cast<uintptr_t*>(WoWOffsets::playerBase);
 *   uint32_t  hp   = *reinterpret_cast<uint32_t*>(base + WoWOffsets::playerHealth);
 */

#ifndef WOWMEMORY_OFFSETS_H
#define WOWMEMORY_OFFSETS_H

#include <cstdint>

namespace WoWOffsets {

// ---------------------------------------------------------------------------
// Arena / Battleground
// ---------------------------------------------------------------------------
constexpr uint32_t arenaPlayer1        = 0xBE9F48;
constexpr uint32_t arenaPlayer2        = arenaPlayer1 + 0x8;
constexpr uint32_t arenaPlayer3        = arenaPlayer2 + 0x8;
constexpr uint32_t arenaPlayer4        = arenaPlayer3 + 0x8;
constexpr uint32_t arenaPlayer5        = arenaPlayer4 + 0x8;
constexpr uint32_t battlegroundStatus  = 0xBEA4D0;
constexpr uint32_t isBattlegroundOver  = 0xBEA588;

// ---------------------------------------------------------------------------
// Character selection / login
// ---------------------------------------------------------------------------
constexpr uint32_t characterSlotSelected = 0x6C436C;

// ---------------------------------------------------------------------------
// Client & UI
// ---------------------------------------------------------------------------
constexpr uint32_t clientGameUITarget   = 0x524BF0;
constexpr uint32_t clientObjectManagerGetActivePlayerObject = 0x4038F0;

// ---------------------------------------------------------------------------
// World / location
// ---------------------------------------------------------------------------
constexpr uint32_t continentName        = 0xCE06D0;   // char* to name string
constexpr uint32_t mapID                = 0xAB63BC;   // uint32 map identifier
constexpr uint32_t zoneID               = 0xBD080C;   // uint32 zone identifier
constexpr uint32_t zoneText             = 0xBD0788;   // char* to zone name
constexpr uint32_t subZoneText          = 0xBD0784;   // char* to sub-zone name
constexpr uint32_t zoneNamePointer      = 0xBD0780;   // pointer to zone name

// ---------------------------------------------------------------------------
// Corpse
// ---------------------------------------------------------------------------
constexpr uint32_t corpseX              = 0xBD0A58;   // float
constexpr uint32_t corpseY              = corpseX + 0x4;
constexpr uint32_t corpseZ              = corpseY + 0x4;
constexpr uint32_t playerCorpseX        = 0xBD0A58;
constexpr uint32_t playerCorpseY        = playerCorpseX + 0x4;
constexpr uint32_t playerCorpseZ        = playerCorpseY + 0x4;

// ---------------------------------------------------------------------------
// CTM (Click-to-Move)
// ---------------------------------------------------------------------------
constexpr uint32_t ctmABase             = 0xCA11D8;
constexpr uint32_t ctmAction            = ctmABase + 0x1C;
constexpr uint32_t ctmDistance          = ctmABase + 0xC;
constexpr uint32_t ctmGUID              = ctmABase + 0x20;
constexpr uint32_t ctmX                 = ctmABase + 0x8C;
constexpr uint32_t ctmY                 = ctmABase + 0x90;
constexpr uint32_t ctmZ                 = ctmABase + 0x94;

// ---------------------------------------------------------------------------
// Connection / session
// ---------------------------------------------------------------------------
constexpr uint32_t currentClientConnection = 0xC79CE0;
constexpr uint32_t currentManagerLocalGUID = 0xC0;  // relative within manager
constexpr uint32_t currentManagerOffset    = 0x2ED0; // relative within connection

// ---------------------------------------------------------------------------
// Device / rendering
// ---------------------------------------------------------------------------
constexpr uint32_t devicePtr1           = 0xC5DF88;
constexpr uint32_t devicePtr2           = 0x397C;    // relative
constexpr uint32_t endScene             = 0xA8;      // relative within device vtable

// ---------------------------------------------------------------------------
// Dynamic objects
// ---------------------------------------------------------------------------
constexpr uint32_t dynamicObjectBytes   = 0x8;       // relative
constexpr uint32_t dynamicObjectCaster  = 0x6;       // relative
constexpr uint32_t dynamicObjectCastTime= 0xB;       // relative
constexpr uint32_t dynamicObjectRadius  = 0xA;       // relative
constexpr uint32_t dynamicObjectSpellID = 0x9;       // relative

// ---------------------------------------------------------------------------
// Object manager / enumeration
// ---------------------------------------------------------------------------
constexpr uint32_t firstObjectOffset    = 0xAC;      // relative within obj manager
constexpr uint32_t nextObjectOffset     = 0x3C;      // relative within each object

// ---------------------------------------------------------------------------
// GameObject descriptors  (aliases — see objectGUID / objectType below)
// ---------------------------------------------------------------------------
constexpr uint32_t gameobjectGUIDOffset = 0x30;      // relative (same as objectGUID)
constexpr uint32_t gameobjectTypeOffset = 0x14;      // relative (same as objectType)

// ---------------------------------------------------------------------------
// Game state
// ---------------------------------------------------------------------------
constexpr uint32_t gameState            = 0xB6A9E0;  // uint32
constexpr uint32_t isLoading            = 0xB6AA30;  // uint8/bool
constexpr uint32_t isIndoor             = 0xB4AA94;  // uint8/bool
constexpr uint32_t worldLoaded          = 0xBEBA40;  // uint8/bool

// ---------------------------------------------------------------------------
// Local player
// ---------------------------------------------------------------------------
constexpr uint32_t localComboPoint         = 0xBD0845;
constexpr uint32_t localLastTarget         = 0xBD07B0;
constexpr uint32_t localLootWindowOpen     = 0xBFA8D0;
constexpr uint32_t localMouseoverGUID      = 0xBD07B0;
constexpr uint32_t localPlayerCharacterState        = 0x6DACA4;
constexpr uint32_t localPlayerCharacterStateOffset1 = 0xC;   // relative
constexpr uint32_t localPlayerCharacterStateOffset2 = 0x94;  // relative
constexpr uint32_t localPlayerCharacterStateOffset3 = 0x90;  // relative
constexpr uint32_t localPlayerGUID         = 0xCA1238;  // uint64
constexpr uint32_t localTargetGUID         = 0xBD07B0;  // uint64
constexpr uint32_t playerBase              = 0xCD87A8;  // pointer to player object
constexpr uint32_t playerHealth            = 0x19B8;    // relative within player object (uint32)
constexpr uint32_t playerIsIngame          = 0xBD0792;  // uint8/bool
constexpr uint32_t playerIsLoadingscreen   = 0xB6AA38;  // uint8/bool
constexpr uint32_t playerName              = 0xC79D18;  // char array (max 12 + null)

// ---------------------------------------------------------------------------
// Lua — C API function addresses
// ---------------------------------------------------------------------------
constexpr uint32_t luaDoString             = 0x819210;  // FrameScript_Execute(code, src, 0)
constexpr uint32_t luaGetLocalizedText     = 0x7225E0;
constexpr uint32_t luaState                = 0x00D3F78C; // pointer to lua_State*
constexpr int32_t  luaGlobalsIndex         = -10002;    // LUA_GLOBALSINDEX pseudo-index

// Individual lua_* function pointers (call via cast to matching signature)
constexpr uint32_t luaGetTop               = 0x0084DBD0; // lua_gettop(L) -> int
constexpr uint32_t luaSetTop               = 0x0084DBF0; // lua_settop(L, index) -> void
constexpr uint32_t luaPushString           = 0x0084E350; // lua_pushstring(L, s) -> void
constexpr uint32_t luaPushInteger          = 0x0084E2D0; // lua_pushinteger(L, n) -> void
constexpr uint32_t luaPushNumber           = 0x0084E2A0; // lua_pushnumber(L, n) -> void
constexpr uint32_t luaPushBoolean          = 0x0084E4D0; // lua_pushboolean(L, b) -> void
constexpr uint32_t luaPushCClosure         = 0x0084E400; // lua_pushcclosure(L, fn, n) -> void
constexpr uint32_t luaToLString            = 0x0084E0E0; // lua_tolstring(L, idx, len) -> const char*
constexpr uint32_t luaToNumber             = 0x0084E030; // lua_tonumber(L, idx) -> double
constexpr uint32_t luaToInteger            = 0x0084E070; // lua_tointeger(L, idx) -> int
constexpr uint32_t luaToBoolean            = 0x0044E2C0; // lua_toboolean(L, idx) -> int  (source: 0x0044E2C0 — note low address, may be a typo in origin)
constexpr uint32_t luaToCFunction          = 0x0084E1C0; // lua_tocfunction(L, idx) -> lua_CFunction
constexpr uint32_t luaType                 = 0x0084DEB0; // lua_type(L, idx) -> int
constexpr uint32_t luaPCall                = 0x0084EC50; // lua_pcall(L, nargs, nres, err) -> int

// WoW custom Lua helpers
constexpr uint32_t luaGetFieldByStackKey   = 0x0084F3B0; // WoW: getfield(L, idx) — key on stack
constexpr uint32_t luaSetField             = 0x0084E900; // WoW: setfield(L, idx, key)
constexpr uint32_t luaRawGetHelper         = 0x00854510; // WoW C impl for rawget()
constexpr uint32_t luaGetGlobalStringVar   = 0x00818010; // WoW: getglobal(L, s, char** result) -> bool

// ---------------------------------------------------------------------------
// Name cache
// ---------------------------------------------------------------------------
constexpr uint32_t nameBase                = 0x1C;      // relative within name entry
constexpr uint32_t nameMask                = 0x24;      // relative within name store
constexpr uint32_t nameStore               = 0xC5D938 + 0x8;
constexpr uint32_t nameString              = 0x20;      // relative within name entry
constexpr uint32_t nameNodeNextOffset      = 0xC;       // relative: next pointer in name node

// ---------------------------------------------------------------------------
// Party
// ---------------------------------------------------------------------------
constexpr uint32_t partyLeader             = 0xBD1968;
constexpr uint32_t partyPlayer1            = 0xBD1948;
constexpr uint32_t partyPlayer2            = partyPlayer1 + 0x8;
constexpr uint32_t partyPlayer3            = partyPlayer2 + 0x8;
constexpr uint32_t partyPlayer4            = partyPlayer3 + 0x8;

// ---------------------------------------------------------------------------
// Pet
// ---------------------------------------------------------------------------
constexpr uint32_t petGUID                 = 0xC234D0;

// ---------------------------------------------------------------------------
// Realm / account
// ---------------------------------------------------------------------------
constexpr uint32_t realmName               = 0xC79B9E;  // char array

// ---------------------------------------------------------------------------
// Movement
// ---------------------------------------------------------------------------
constexpr uint32_t sendMovementPacket      = 0x7413F0;
constexpr uint32_t setFacing               = 0x9606E0;

// ---------------------------------------------------------------------------
// Casting
// ---------------------------------------------------------------------------
constexpr uint32_t staticCastingstate      = 0x6F5250;

// ---------------------------------------------------------------------------
// Timing
// ---------------------------------------------------------------------------
constexpr uint32_t tickCount               = 0xB499A4;  // uint32
constexpr uint32_t timestamp               = 0xB1D618;  // uint32

// ---------------------------------------------------------------------------
// Chat
// ---------------------------------------------------------------------------
constexpr uint32_t wowChat                 = 0xB75A60;
constexpr uint32_t wowChatNextMsg          = 0x17C0;    // relative

// ---------------------------------------------------------------------------
// Object properties  (relative to object base address)
// ---------------------------------------------------------------------------
constexpr uint32_t objectType              = 0x14;      // relative
constexpr uint32_t objectGUID              = 0x30;      // relative
constexpr uint32_t objectUnitFields        = 0x8;       // relative: ptr to unit descriptor array
constexpr uint32_t objectDescriptorOffset  = 0x8;       // relative: ptr to display/power descriptor
constexpr uint32_t objectPosX              = 0x79C;     // relative: float
constexpr uint32_t objectPosY              = 0x798;     // relative: float
constexpr uint32_t objectPosZ              = 0x7A0;     // relative: float
constexpr uint32_t objectRotation          = 0x7A8;     // relative: float (facing radians)

// ---------------------------------------------------------------------------
// Unit field descriptor offsets  (relative to UnitFields pointer, in bytes)
// ---------------------------------------------------------------------------
constexpr uint32_t unitFieldHealth         = 0x18 * 4;  // current health
constexpr uint32_t unitFieldMaxHealth      = 0x20 * 4;  // max health
constexpr uint32_t unitFieldLevel          = 0x36 * 4;  // unit level
constexpr uint32_t unitFieldPowers         = 0x4C;      // start of current power[7] array
constexpr uint32_t unitFieldMaxPowers      = 0x6C;      // start of max power[7] array
constexpr uint32_t unitFieldEnergy         = 0x19 * 4;  // mana/rage/energy (index by power type)
constexpr uint32_t unitFieldMaxEnergy      = 0x21 * 4;  // max mana/rage/energy
constexpr uint32_t unitFieldMaxPower1      = 0x21 * 4;
constexpr uint32_t unitFieldMaxPower2      = 0x22 * 4;
constexpr uint32_t unitFieldMaxPower3      = 0x23 * 4;
constexpr uint32_t unitFieldMaxPower4      = 0x24 * 4;
constexpr uint32_t unitFieldMaxPower5      = 0x25 * 4;
constexpr uint32_t unitFieldMaxPower6      = 0x26 * 4;
constexpr uint32_t unitFieldMaxPower7      = 0x27 * 4;
constexpr uint32_t unitFieldSummonedBy     = 0xE * 4;   // summoner GUID
constexpr uint32_t unitFieldBytes0         = 0x5C;      // class/race byte pack
constexpr uint32_t unitFieldFlags          = 0xEC;      // unit flags
constexpr uint32_t unitFieldTargetGUID     = 0x12 * 4;  // current target GUID
constexpr uint32_t unitFieldPowerTypeByteFromDescriptor = 0x47; // power type byte in descriptor

// ---------------------------------------------------------------------------
// Spells & spellbook
// ---------------------------------------------------------------------------
constexpr uint32_t spellCastSpell                  = 0x0080DA40;  // CGGameUI::CastSpell(spellId, 0)
constexpr uint32_t spellBookStartAddress           = 0x00BE5D88;
constexpr uint32_t spellBookSpellCountAddress      = 0x00BE8D9C;
constexpr uint32_t spellBookSlotMapAddress         = 0x00BE6D88;
constexpr uint32_t spellBookKnownSpellCountAddress = 0x00BE8D98;

// ---------------------------------------------------------------------------
// Cooldowns
// ---------------------------------------------------------------------------
constexpr uint32_t spellCooldownPtr                = 0x00D3F5AC;  // ptr to cooldown struct
constexpr uint32_t spellCGetSpellCooldown          = 0x00807980;  // GetSpellCooldown(spellId)
constexpr uint32_t spellCGetSpellRange             = 0x00802C30;  // GetSpellRange(spellId, ...)

// ---------------------------------------------------------------------------
// Additional GUIDs / globals
// ---------------------------------------------------------------------------
constexpr uint32_t lastTargetGUID                  = 0x00BD07B8;
constexpr uint32_t mouseOverGUID                   = 0x00BD07A0;
constexpr uint32_t comboPoints                     = 0x00BD084D;  // player combo points byte
constexpr uint32_t lastHardwareActionTimestamp     = 0x00B499A4;

// ---------------------------------------------------------------------------
// Casting / channeling  (relative to object base address)
// ---------------------------------------------------------------------------
constexpr uint32_t objectCastingSpellId    = 0xA6C;     // relative
constexpr uint32_t objectChannelSpellId    = 0xA80;     // relative
constexpr uint32_t unitCastingIdOffset     = 0xC08;     // relative: current cast spell id
constexpr uint32_t unitChannelIdOffset     = 0xC20;     // relative: current channel spell id

// ---------------------------------------------------------------------------
// Auras  (relative to unit base address)
// ---------------------------------------------------------------------------
constexpr uint32_t auraCount1Offset        = 0xDD0;     // relative: aura count (table 1)
constexpr uint32_t auraCount2Offset        = 0xC54;     // relative: aura count (table 2)
constexpr uint32_t auraTable1Offset        = 0xC50;     // relative: ptr to aura table 1
constexpr uint32_t auraTable2Offset        = 0xC58;     // relative: ptr to aura table 2
constexpr uint32_t auraStructSize          = 0x18;      // size of one aura entry in bytes
constexpr uint32_t auraStructSpellIdOffset = 0x8;       // relative within aura entry: spell id

// ---------------------------------------------------------------------------
// Combat log
// ---------------------------------------------------------------------------
constexpr uint32_t combatLogListManager        = 0xADB974;
constexpr uint32_t combatLogListHeadOffset     = 0x0;   // relative within manager: head ptr
constexpr uint32_t combatLogListTailOffset     = 0x4;   // relative within manager: tail ptr
constexpr uint32_t combatLogEventPrevOffset    = 0x0;   // relative within node: prev ptr
constexpr uint32_t combatLogEventNextOffset    = 0x4;   // relative within node: next ptr
constexpr uint32_t combatLogEventTimestampOffset = 0x8; // relative within node: timestamp
constexpr uint32_t combatLogTimestampSource    = 0x00CD76AC;
constexpr uint32_t combatLogNextUnprocessedNode= 0x00CA1394;

// ---------------------------------------------------------------------------
// Camera
// ---------------------------------------------------------------------------
constexpr uint32_t cameraBasePtrOffset     = 0x00C7B5A8; // absolute: ptr chain start
constexpr uint32_t cameraOffset1           = 0x6B04;     // relative: first dereference step
constexpr uint32_t cameraOffset2           = 0xE8;       // relative: second dereference step
constexpr uint32_t cameraPitchOffset       = 0x34;       // relative within camera struct: pitch
constexpr uint32_t cameraYawOffset         = 0x30;       // relative within camera struct: yaw

} // namespace WoWOffsets

#endif // WOWMEMORY_OFFSETS_H
