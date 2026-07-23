#include "WowDataModel.h"

#include <cstdio>

namespace WoTLKGuiLayer {

bool WowDataModel::Initialise(Rml::Context* context)
{
    Rml::DataModelConstructor c = context->CreateDataModel("wow");
    if (!c) {
        fprintf(stderr, "[WowDataModel] Failed to create data model 'wow'\n");
        return false;
    }

    // Identity
    c.Bind("playerName",    &m_playerName);
    c.Bind("realmName",     &m_realmName);

    // World
    c.Bind("continentName", &m_continentName);
    c.Bind("zoneText",      &m_zoneText);
    c.Bind("subZoneText",   &m_subZoneText);
    c.Bind("mapID",         &m_mapID);
    c.Bind("zoneID",        &m_zoneID);

    // Player state
    c.Bind("playerHealth",    &m_playerHealth);
    c.Bind("playerMaxHealth", &m_playerMaxHealth);
    c.Bind("playerLevel",     &m_playerLevel);
    c.Bind("playerPower",     &m_playerPower);
    c.Bind("playerMaxPower",  &m_playerMaxPower);
    c.Bind("playerPowerType", &m_playerPowerType);
    c.Bind("playerIsIngame",  &m_playerIsIngame);

    // Party / Raid / Quest metrics
    c.Bind("numPartyMembers",   &m_numPartyMembers);
    c.Bind("partyDifficulty",   &m_partyDifficulty);
    c.Bind("numRaidMembers",    &m_numRaidMembers);
    c.Bind("raidDifficulty",    &m_raidDifficulty);
    c.Bind("activeQuestsCount", &m_activeQuestsCount);

    // Target state
    c.Bind("targetHealth",    &m_targetHealth);
    c.Bind("targetMaxHealth", &m_targetMaxHealth);
    c.Bind("targetPower",     &m_targetPower);
    c.Bind("targetMaxPower",  &m_targetMaxPower);
    c.Bind("targetPowerType", &m_targetPowerType);
    c.Bind("targetLevel",     &m_targetLevel);
    c.Bind("targetName",      &m_targetName);

    // Game state
    c.Bind("gameState",   &m_gameState);
    c.Bind("worldLoaded", &m_worldLoaded);
    c.Bind("isLoading",   &m_isLoading);
    c.Bind("isIndoor",    &m_isIndoor);
    c.Bind("tickCount",   &m_tickCount);

    // Corpse position
    c.Bind("corpseX", &m_corpseX);
    c.Bind("corpseY", &m_corpseY);
    c.Bind("corpseZ", &m_corpseZ);

    // Register arrays for auras and combat log
    c.RegisterArray<Rml::Vector<int>>();
    c.RegisterArray<Rml::Vector<Rml::String>>();

    c.Bind("playerAuras", &m_playerAuras);
    c.Bind("targetAuras", &m_targetAuras);
    c.Bind("combatLog", &m_combatLogHistory);

    // Register custom group member struct & arrays for RmlUi data model
    if (auto member_handle = c.RegisterStruct<RmlGroupMember>()) {
        member_handle.RegisterMember("guid", &RmlGroupMember::guid);
        member_handle.RegisterMember("name", &RmlGroupMember::name);
        member_handle.RegisterMember("health", &RmlGroupMember::health);
        member_handle.RegisterMember("maxHealth", &RmlGroupMember::maxHealth);
        member_handle.RegisterMember("auras", &RmlGroupMember::auras);
    }
    c.RegisterArray<Rml::Vector<RmlGroupMember>>();

    c.Bind("partyMembers", &m_partyMembers);
    c.Bind("raidMembers", &m_raidMembers);

    m_handle = c.GetModelHandle();
    return true;
}

void WowDataModel::Update(const WoWMemory::GameData& data)
{
    if (!m_handle) return;

#define DIRTY_IF_CHANGED(field, name, value)  \
    do {                                      \
        if ((field) != (value)) {             \
            (field) = (value);                \
            m_handle.DirtyVariable(name);     \
        }                                     \
    } while (0)

    // Strings: compare via operator!=
    if (m_playerName != data.playerName) {
        m_playerName = data.playerName;
        m_handle.DirtyVariable("playerName");
    }
    if (m_realmName != data.realmName) {
        m_realmName = data.realmName;
        m_handle.DirtyVariable("realmName");
    }
    if (m_continentName != data.continentName) {
        m_continentName = data.continentName;
        m_handle.DirtyVariable("continentName");
    }
    if (m_zoneText != data.zoneText) {
        m_zoneText = data.zoneText;
        m_handle.DirtyVariable("zoneText");
    }
    if (m_subZoneText != data.subZoneText) {
        m_subZoneText = data.subZoneText;
        m_handle.DirtyVariable("subZoneText");
    }
    if (m_targetName != data.targetName) {
        m_targetName = data.targetName;
        m_handle.DirtyVariable("targetName");
    }

    // Numeric / bool fields — keep internal mirrors as int
    DIRTY_IF_CHANGED(m_mapID,          "mapID",          static_cast<int>(data.mapID));
    DIRTY_IF_CHANGED(m_zoneID,         "zoneID",         static_cast<int>(data.zoneID));
    DIRTY_IF_CHANGED(m_gameState,      "gameState",      static_cast<int>(data.gameState));
    DIRTY_IF_CHANGED(m_worldLoaded,    "worldLoaded",    data.worldLoaded  ? 1 : 0);
    DIRTY_IF_CHANGED(m_isLoading,      "isLoading",      data.isLoading    ? 1 : 0);
    DIRTY_IF_CHANGED(m_isIndoor,       "isIndoor",       data.isIndoor     ? 1 : 0);
    DIRTY_IF_CHANGED(m_playerIsIngame, "playerIsIngame", data.playerIsIngame ? 1 : 0);
    DIRTY_IF_CHANGED(m_playerHealth,   "playerHealth",   static_cast<int>(data.playerHealth));
    DIRTY_IF_CHANGED(m_playerMaxHealth,"playerMaxHealth",static_cast<int>(data.playerMaxHealth));
    DIRTY_IF_CHANGED(m_playerLevel,    "playerLevel",    static_cast<int>(data.playerLevel));
    DIRTY_IF_CHANGED(m_playerPower,     "playerPower",     static_cast<int>(data.playerPower));
    DIRTY_IF_CHANGED(m_playerMaxPower,  "playerMaxPower",  static_cast<int>(data.playerMaxPower));
    DIRTY_IF_CHANGED(m_playerPowerType, "playerPowerType", static_cast<int>(data.playerPowerType));

    // Party / Raid / Quest metrics
    DIRTY_IF_CHANGED(m_numPartyMembers,   "numPartyMembers",   static_cast<int>(data.numPartyMembers));
    DIRTY_IF_CHANGED(m_partyDifficulty,   "partyDifficulty",   static_cast<int>(data.partyDifficulty));
    DIRTY_IF_CHANGED(m_numRaidMembers,    "numRaidMembers",    static_cast<int>(data.numRaidMembers));
    DIRTY_IF_CHANGED(m_raidDifficulty,    "raidDifficulty",    static_cast<int>(data.raidDifficulty));
    DIRTY_IF_CHANGED(m_activeQuestsCount, "activeQuestsCount", static_cast<int>(data.activeQuestsCount));

    DIRTY_IF_CHANGED(m_targetHealth,    "targetHealth",    static_cast<int>(data.targetHealth));
    DIRTY_IF_CHANGED(m_targetMaxHealth, "targetMaxHealth", static_cast<int>(data.targetMaxHealth));
    DIRTY_IF_CHANGED(m_targetPower,     "targetPower",     static_cast<int>(data.targetPower));
    DIRTY_IF_CHANGED(m_targetMaxPower,  "targetMaxPower",  static_cast<int>(data.targetMaxPower));
    DIRTY_IF_CHANGED(m_targetPowerType, "targetPowerType", static_cast<int>(data.targetPowerType));
    DIRTY_IF_CHANGED(m_targetLevel,     "targetLevel",     static_cast<int>(data.targetLevel));

    DIRTY_IF_CHANGED(m_tickCount,      "tickCount",      static_cast<int>(data.tickCount));

    if (m_corpseX != data.corpseX) { m_corpseX = data.corpseX; m_handle.DirtyVariable("corpseX"); }
    if (m_corpseY != data.corpseY) { m_corpseY = data.corpseY; m_handle.DirtyVariable("corpseY"); }
    if (m_corpseZ != data.corpseZ) { m_corpseZ = data.corpseZ; m_handle.DirtyVariable("corpseZ"); }

    // Update Player Auras
    bool playerAurasChanged = false;
    if (m_playerAuras.size() != data.auraCount) {
        playerAurasChanged = true;
    } else {
        for (uint32_t i = 0; i < data.auraCount; ++i) {
            if (m_playerAuras[i] != static_cast<int>(data.auras[i].spellId)) {
                playerAurasChanged = true;
                break;
            }
        }
    }
    if (playerAurasChanged) {
        m_playerAuras.clear();
        for (uint32_t i = 0; i < data.auraCount; ++i) {
            m_playerAuras.push_back(static_cast<int>(data.auras[i].spellId));
        }
        m_handle.DirtyVariable("playerAuras");
    }

    // Update Target Auras
    bool targetAurasChanged = false;
    if (m_targetAuras.size() != data.targetAuraCount) {
        targetAurasChanged = true;
    } else {
        for (uint32_t i = 0; i < data.targetAuraCount; ++i) {
            if (m_targetAuras[i] != static_cast<int>(data.targetAuras[i].spellId)) {
                targetAurasChanged = true;
                break;
            }
        }
    }
    if (targetAurasChanged) {
        m_targetAuras.clear();
        for (uint32_t i = 0; i < data.targetAuraCount; ++i) {
            m_targetAuras.push_back(static_cast<int>(data.targetAuras[i].spellId));
        }
        m_handle.DirtyVariable("targetAuras");
    }

    // Update Party & Raid list bindings
    auto updateGroupList = [](Rml::Vector<RmlGroupMember>& rmlList, const std::vector<WoWMemory::GroupMemberData>& rawList) -> bool {
        bool changed = false;
        if (rmlList.size() != rawList.size()) {
            changed = true;
        } else {
            for (size_t i = 0; i < rawList.size(); ++i) {
                const auto& raw = rawList[i];
                const auto& rml = rmlList[i];
                char rawGuidBuf[32];
                snprintf(rawGuidBuf, sizeof(rawGuidBuf), "0x%llX", (unsigned long long)raw.guid);
                if (rml.guid != rawGuidBuf ||
                    rml.name != raw.name ||
                    rml.health != static_cast<int>(raw.health) ||
                    rml.maxHealth != static_cast<int>(raw.maxHealth) ||
                    rml.auras.size() != raw.auraCount) {
                    changed = true;
                    break;
                }
                for (uint32_t j = 0; j < raw.auraCount; ++j) {
                    if (rml.auras[j] != static_cast<int>(raw.auras[j].spellId)) {
                        changed = true;
                        break;
                    }
                }
                if (changed) break;
            }
        }

        if (changed) {
            rmlList.clear();
            for (const auto& raw : rawList) {
                RmlGroupMember rml;
                char guidBuf[32];
                snprintf(guidBuf, sizeof(guidBuf), "0x%llX", (unsigned long long)raw.guid);
                rml.guid = guidBuf;
                rml.name = raw.name;
                rml.health = static_cast<int>(raw.health);
                rml.maxHealth = static_cast<int>(raw.maxHealth);
                for (uint32_t j = 0; j < raw.auraCount; ++j) {
                    rml.auras.push_back(static_cast<int>(raw.auras[j].spellId));
                }
                rmlList.push_back(rml);
            }
        }
        return changed;
    };

    if (updateGroupList(m_partyMembers, data.partyMembersList)) {
        m_handle.DirtyVariable("partyMembers");
    }
    if (updateGroupList(m_raidMembers, data.raidMembersList)) {
        m_handle.DirtyVariable("raidMembers");
    }

    // Update Combat Log History
    if (data.combatLogEventCount > 0) {
        for (uint32_t i = 0; i < data.combatLogEventCount; ++i) {
            const auto& ev = data.combatLogEvents[i];

            char buf[256];
            std::string critStr = (ev.flags & 1) ? " (Crit!)" : "";

            snprintf(buf, sizeof(buf), "[%u] Type:%d | Src:0x%llX -> Dst:0x%llX | Amt:%d%s",
                     ev.timestamp,
                     ev.eventTypeId,
                     (unsigned long long)ev.sourceGuid,
                     (unsigned long long)ev.destGuid,
                     ev.amount,
                     critStr.c_str());

            m_combatLogHistory.push_back(buf);
        }

        constexpr size_t kMaxHistory = 30;
        if (m_combatLogHistory.size() > kMaxHistory) {
            m_combatLogHistory.erase(m_combatLogHistory.begin(),
                                     m_combatLogHistory.begin() + (m_combatLogHistory.size() - kMaxHistory));
        }
        m_handle.DirtyVariable("combatLog");
    }

#undef DIRTY_IF_CHANGED
}

void WowDataModel::Shutdown()
{
    m_handle = {};
}

} // namespace WoTLKGuiLayer
