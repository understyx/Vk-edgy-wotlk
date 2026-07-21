#pragma once

#include <RmlUi/Core.h>
#include "wowmemory/wowmemory.h"

namespace WoTLKGuiLayer {

/**
 * @class WowDataModel
 * @brief Binds live WoW game data to an RmlUi data model named "wow".
 *
 * Once initialised against an Rml::Context, any RML document that declares
 *   <body data-model="wow">
 * can bind individual fields with data-value attributes, e.g.:
 *   <span data-value="playerName"></span>
 *   <span data-value="zoneText"></span>
 *
 * Call Update() every frame before Rml::Context::Update() to push new values.
 */
class WowDataModel {
public:
    WowDataModel()  = default;
    ~WowDataModel() = default;

    /**
     * @brief Initialise the data model on the given context.
     * @return true on success.
     */
    bool Initialise(Rml::Context* context);

    /**
     * @brief Push a new GameData snapshot into the model.
     *
     * Marks only changed variables as dirty so RmlUi can skip unchanged nodes.
     */
    void Update(const WoWMemory::GameData& data);

    /** @brief Remove the model from the context and free resources. */
    void Shutdown();

private:
    Rml::DataModelHandle m_handle;

    // Mirror of GameData with RmlUi-compatible types.
    // All strings use Rml::String (= std::string) and booleans are kept as
    // int because RmlUi's data binding converts int → "0"/"1" reliably.
    Rml::String m_playerName;
    Rml::String m_realmName;
    Rml::String m_continentName;
    Rml::String m_zoneText;
    Rml::String m_subZoneText;
    int         m_mapID         = 0;
    int         m_zoneID        = 0;
    int         m_gameState     = 0;
    int         m_worldLoaded   = 0;
    int         m_isLoading     = 0;
    int         m_isIndoor      = 0;
    int         m_playerIsIngame= 0;
    int         m_playerHealth  = 0;
    int         m_tickCount     = 0;
    float       m_corpseX       = 0.0f;
    float       m_corpseY       = 0.0f;
    float       m_corpseZ       = 0.0f;
};

} // namespace WoTLKGuiLayer
