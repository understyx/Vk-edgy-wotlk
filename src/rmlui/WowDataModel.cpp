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

    m_handle = c.GetModelHandle();
    return true;
}

void WowDataModel::Update(const WoWMemory::GameData& data)
{
    if (!m_handle) return;

#define DIRTY_IF_CHANGED(field, value)        \
    do {                                      \
        if ((field) != (value)) {             \
            (field) = (value);                \
            m_handle.DirtyVariable(#field);   \
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
    DIRTY_IF_CHANGED(m_mapID,          static_cast<int>(data.mapID));
    DIRTY_IF_CHANGED(m_zoneID,         static_cast<int>(data.zoneID));
    DIRTY_IF_CHANGED(m_gameState,      static_cast<int>(data.gameState));
    DIRTY_IF_CHANGED(m_worldLoaded,    data.worldLoaded  ? 1 : 0);
    DIRTY_IF_CHANGED(m_isLoading,      data.isLoading    ? 1 : 0);
    DIRTY_IF_CHANGED(m_isIndoor,       data.isIndoor     ? 1 : 0);
    DIRTY_IF_CHANGED(m_playerIsIngame, data.playerIsIngame ? 1 : 0);
    DIRTY_IF_CHANGED(m_playerHealth,   static_cast<int>(data.playerHealth));
    DIRTY_IF_CHANGED(m_tickCount,      static_cast<int>(data.tickCount));

    if (m_corpseX != data.corpseX) { m_corpseX = data.corpseX; m_handle.DirtyVariable("corpseX"); }
    if (m_corpseY != data.corpseY) { m_corpseY = data.corpseY; m_handle.DirtyVariable("corpseY"); }
    if (m_corpseZ != data.corpseZ) { m_corpseZ = data.corpseZ; m_handle.DirtyVariable("corpseZ"); }

#undef DIRTY_IF_CHANGED
}

void WowDataModel::Shutdown()
{
    m_handle = {};
}

} // namespace WoTLKGuiLayer
