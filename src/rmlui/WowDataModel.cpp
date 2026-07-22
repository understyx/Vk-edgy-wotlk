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
    c.Bind("playerHealth",   &m_playerHealth);
    c.Bind("playerIsIngame", &m_playerIsIngame);

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

    // Numeric / bool fields — keep internal mirrors as int
    DIRTY_IF_CHANGED(m_mapID,          "mapID",          static_cast<int>(data.mapID));
    DIRTY_IF_CHANGED(m_zoneID,         "zoneID",         static_cast<int>(data.zoneID));
    DIRTY_IF_CHANGED(m_gameState,      "gameState",      static_cast<int>(data.gameState));
    DIRTY_IF_CHANGED(m_worldLoaded,    "worldLoaded",    data.worldLoaded  ? 1 : 0);
    DIRTY_IF_CHANGED(m_isLoading,      "isLoading",      data.isLoading    ? 1 : 0);
    DIRTY_IF_CHANGED(m_isIndoor,       "isIndoor",       data.isIndoor     ? 1 : 0);
    DIRTY_IF_CHANGED(m_playerIsIngame, "playerIsIngame", data.playerIsIngame ? 1 : 0);
    DIRTY_IF_CHANGED(m_playerHealth,   "playerHealth",   static_cast<int>(data.playerHealth));
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
