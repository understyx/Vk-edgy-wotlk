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
// GameObject descriptors
// ---------------------------------------------------------------------------
constexpr uint32_t gameobjectGUIDOffset = 0x30;      // relative
constexpr uint32_t gameobjectTypeOffset = 0x14;      // relative

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
// Lua
// ---------------------------------------------------------------------------
constexpr uint32_t luaDoString             = 0x819210;
constexpr uint32_t luaGetLocalizedText     = 0x7225E0;

// ---------------------------------------------------------------------------
// Name cache
// ---------------------------------------------------------------------------
constexpr uint32_t nameBase                = 0x1C;      // relative within name entry
constexpr uint32_t nameMask                = 0x24;      // relative within name store
constexpr uint32_t nameStore               = 0xC5D938 + 0x8;
constexpr uint32_t nameString              = 0x20;      // relative within name entry

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

} // namespace WoWOffsets

#endif // WOWMEMORY_OFFSETS_H
