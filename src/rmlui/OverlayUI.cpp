#include "OverlayUI.h"
#include "wowmemory/offsets.h"
#include <cstdio>
#include <cstring>

#ifdef RMLUI_LUA_BINDINGS
#include <RmlUi/Lua.h>
extern "C" {
#include <lua.h>
#include <lauxlib.h>
}
#endif

namespace WoTLKGuiLayer {

OverlayUI::~OverlayUI()
{
    if (m_ready)
        Shutdown();
}

bool OverlayUI::Initialize(const InitInfo& info)
{
    m_device   = info.device;
    m_dispatch = info.dispatch;

    // ---- RmlUi interfaces ----
    m_sysInterface  = std::make_unique<RmlSystemInterface>();
    m_fileInterface = std::make_unique<RmlFileInterface>(info.uiDir);

    m_renderer = std::make_unique<VulkanRenderer>();

    VulkanRenderer::InitInfo ri{};
    ri.device                    = info.device;
    ri.physicalDevice            = info.physicalDevice;
    ri.dispatch                  = info.dispatch;
    ri.graphicsQueueFamilyIndex  = info.graphicsQueueFamilyIndex;
    ri.shaderDir                 = info.shaderDir;

    if (!m_renderer->Initialize(ri)) {
        fprintf(stderr, "[RmlUi] VulkanRenderer::Initialize failed\n");
        return false;
    }

    // Provide the graphics queue so the renderer can upload textures
    m_renderer->SetupWithQueue(info.graphicsQueue);

    // ---- Bootstrap RmlUi ----
    Rml::SetSystemInterface(m_sysInterface.get());
    Rml::SetFileInterface(m_fileInterface.get());
    Rml::SetRenderInterface(m_renderer.get());

    if (!Rml::Initialise()) {
        fprintf(stderr, "[RmlUi] Rml::Initialise() failed\n");
        return false;
    }

#ifdef RMLUI_LUA_BINDINGS
    Rml::Lua::Initialise();
#endif

    // Load default font (optional; overlay will render without text if unavailable)
    if (!info.fontPath.empty()) {
        if (!Rml::LoadFontFace(info.fontPath)) {
            fprintf(stderr, "[RmlUi] Warning: could not load font %s\n",
                    info.fontPath.c_str());
        }
    }

    m_ready = true;
    return true;
}

void OverlayUI::Shutdown()
{
    if (!m_ready) return;

    m_dispatch->DeviceWaitIdle(m_device);

    if (m_document) {
        m_document->Close();
        m_document = nullptr;
    }
    if (m_context) {
        m_dataModel.Shutdown();
        Rml::RemoveContext(m_context->GetName());
        m_context = nullptr;
    }

    Rml::Shutdown();

#ifdef RMLUI_LUA_BINDINGS
    Rml::Lua::Shutdown();
#endif

    DestroySyncObjects();
    DestroySwapchainResources();

    if (m_renderer) {
        m_renderer->Shutdown();
        m_renderer.reset();
    }

    m_sysInterface.reset();
    m_fileInterface.reset();
    m_ready = false;
}

// ============================================================================
// Swapchain lifecycle
// ============================================================================

void OverlayUI::ResizeSwapchain(const std::vector<VkImage>& images,
                                 VkFormat                    format,
                                 VkExtent2D                  extent)
{
    if (!m_ready) return;

    m_extent = extent;
    m_renderer->ResizeSwapchain(images, format, extent);

    // Create or recreate the RmlUi context with the new dimensions
    if (m_document) {
        m_document->Close();
        m_document = nullptr;
    }
    if (m_context) {
        m_dataModel.Shutdown();
        Rml::RemoveContext(m_context->GetName());
        m_context = nullptr;
    }

    m_context = Rml::CreateContext("overlay",
        Rml::Vector2i(static_cast<int>(extent.width),
                      static_cast<int>(extent.height)));
    if (!m_context) {
        fprintf(stderr, "[RmlUi] Failed to create RmlUi context\n");
        return;
    }

    // Initialise the WoW data model so HTML documents can bind to it
    m_dataModel.Initialise(m_context);

#ifdef RMLUI_LUA_BINDINGS
    RegisterLuaGlobals();
#endif

    // Load the overlay document
    m_document = m_context->LoadDocument("overlay.rml");
    if (!m_document) {
        fprintf(stderr, "[RmlUi] Failed to load overlay.rml\n");
        return;
    }
    m_document->Show();

    // Recreate sync objects for the new image count
    DestroySyncObjects();
    if (CreateSyncObjects(static_cast<uint32_t>(images.size())) != VK_SUCCESS) {
        fprintf(stderr, "[RmlUi] Failed to create sync objects\n");
    }
}

void OverlayUI::DestroySwapchainResources()
{
    if (m_renderer)
        m_renderer->DestroySwapchainResources();
    DestroySyncObjects();
}

// ============================================================================
// Per-frame render
// ============================================================================

void OverlayUI::UpdateGameData()
{
    if (!m_ready || !m_context) return;

    WoWMemory::GameData snapshot;
    if (m_gameReader.ReadGameData(snapshot))
        m_dataModel.Update(snapshot);

#ifdef RMLUI_LUA_BINDINGS
    // Refresh the `wow` Lua global table with the latest values
    lua_State* L = Rml::Lua::Interpreter::GetLuaState();
    if (L && lua_getglobal(L, "wow") == LUA_TTABLE) {
        lua_pushstring(L, snapshot.playerName.c_str());
        lua_setfield(L, -2, "playerName");
        lua_pushstring(L, snapshot.realmName.c_str());
        lua_setfield(L, -2, "realmName");
        lua_pushstring(L, snapshot.zoneText.c_str());
        lua_setfield(L, -2, "zoneText");
        lua_pushstring(L, snapshot.subZoneText.c_str());
        lua_setfield(L, -2, "subZoneText");
        lua_pushstring(L, snapshot.continentName.c_str());
        lua_setfield(L, -2, "continentName");
        lua_pushinteger(L, static_cast<lua_Integer>(snapshot.mapID));
        lua_setfield(L, -2, "mapID");
        lua_pushinteger(L, static_cast<lua_Integer>(snapshot.zoneID));
        lua_setfield(L, -2, "zoneID");
        lua_pushinteger(L, static_cast<lua_Integer>(snapshot.playerHealth));
        lua_setfield(L, -2, "playerHealth");
        lua_pushboolean(L, snapshot.playerIsIngame ? 1 : 0);
        lua_setfield(L, -2, "playerIsIngame");
        lua_pushboolean(L, snapshot.worldLoaded ? 1 : 0);
        lua_setfield(L, -2, "worldLoaded");
        lua_pushboolean(L, snapshot.isLoading ? 1 : 0);
        lua_setfield(L, -2, "isLoading");
        lua_pushboolean(L, snapshot.isIndoor ? 1 : 0);
        lua_setfield(L, -2, "isIndoor");
        lua_pushinteger(L, static_cast<lua_Integer>(snapshot.gameState));
        lua_setfield(L, -2, "gameState");
        lua_pushinteger(L, static_cast<lua_Integer>(snapshot.tickCount));
        lua_setfield(L, -2, "tickCount");
        lua_pushnumber(L, static_cast<lua_Number>(snapshot.corpseX));
        lua_setfield(L, -2, "corpseX");
        lua_pushnumber(L, static_cast<lua_Number>(snapshot.corpseY));
        lua_setfield(L, -2, "corpseY");
        lua_pushnumber(L, static_cast<lua_Number>(snapshot.corpseZ));
        lua_setfield(L, -2, "corpseZ");
    }
    lua_settop(L, 0); // clear stack
#endif
}

#ifdef RMLUI_LUA_BINDINGS
void OverlayUI::RegisterLuaGlobals()
{
    lua_State* L = Rml::Lua::Interpreter::GetLuaState();
    if (!L) return;

    // ---- wow (live game data) ----
    lua_newtable(L);
    // Seed with empty/zero values; UpdateGameData() fills them each frame.
    const char* strFields[] = {
        "playerName", "realmName", "zoneText", "subZoneText", "continentName", nullptr
    };
    for (int i = 0; strFields[i]; ++i) {
        lua_pushstring(L, "");
        lua_setfield(L, -2, strFields[i]);
    }
    const char* intFields[] = {
        "mapID", "zoneID", "playerHealth", "gameState", "tickCount", nullptr
    };
    for (int i = 0; intFields[i]; ++i) {
        lua_pushinteger(L, 0);
        lua_setfield(L, -2, intFields[i]);
    }
    const char* boolFields[] = {
        "playerIsIngame", "worldLoaded", "isLoading", "isIndoor", nullptr
    };
    for (int i = 0; boolFields[i]; ++i) {
        lua_pushboolean(L, 0);
        lua_setfield(L, -2, boolFields[i]);
    }
    const char* numFields[] = { "corpseX", "corpseY", "corpseZ", nullptr };
    for (int i = 0; numFields[i]; ++i) {
        lua_pushnumber(L, 0.0);
        lua_setfield(L, -2, numFields[i]);
    }
    lua_setglobal(L, "wow");

    // ---- wow_offsets (raw address constants) ----
    lua_newtable(L);

#define PUSH_OFFSET(name) \
    lua_pushinteger(L, static_cast<lua_Integer>(WoWOffsets::name)); \
    lua_setfield(L, -2, #name)

    PUSH_OFFSET(arenaPlayer1);
    PUSH_OFFSET(arenaPlayer2);
    PUSH_OFFSET(arenaPlayer3);
    PUSH_OFFSET(arenaPlayer4);
    PUSH_OFFSET(arenaPlayer5);
    PUSH_OFFSET(battlegroundStatus);
    PUSH_OFFSET(characterSlotSelected);
    PUSH_OFFSET(clientGameUITarget);
    PUSH_OFFSET(continentName);
    PUSH_OFFSET(corpseX);
    PUSH_OFFSET(corpseY);
    PUSH_OFFSET(corpseZ);
    PUSH_OFFSET(ctmABase);
    PUSH_OFFSET(ctmAction);
    PUSH_OFFSET(ctmDistance);
    PUSH_OFFSET(ctmGUID);
    PUSH_OFFSET(ctmX);
    PUSH_OFFSET(ctmY);
    PUSH_OFFSET(ctmZ);
    PUSH_OFFSET(currentClientConnection);
    PUSH_OFFSET(currentManagerLocalGUID);
    PUSH_OFFSET(currentManagerOffset);
    PUSH_OFFSET(devicePtr1);
    PUSH_OFFSET(devicePtr2);
    PUSH_OFFSET(dynamicObjectBytes);
    PUSH_OFFSET(dynamicObjectCaster);
    PUSH_OFFSET(dynamicObjectCastTime);
    PUSH_OFFSET(dynamicObjectRadius);
    PUSH_OFFSET(dynamicObjectSpellID);
    PUSH_OFFSET(endScene);
    PUSH_OFFSET(firstObjectOffset);
    PUSH_OFFSET(gameobjectGUIDOffset);
    PUSH_OFFSET(gameobjectTypeOffset);
    PUSH_OFFSET(gameState);
    PUSH_OFFSET(isBattlegroundOver);
    PUSH_OFFSET(isLoading);
    PUSH_OFFSET(isIndoor);
    PUSH_OFFSET(localComboPoint);
    PUSH_OFFSET(localLastTarget);
    PUSH_OFFSET(localLootWindowOpen);
    PUSH_OFFSET(localMouseoverGUID);
    PUSH_OFFSET(localPlayerCharacterState);
    PUSH_OFFSET(localPlayerGUID);
    PUSH_OFFSET(localTargetGUID);
    PUSH_OFFSET(luaDoString);
    PUSH_OFFSET(luaGetLocalizedText);
    PUSH_OFFSET(mapID);
    PUSH_OFFSET(nameBase);
    PUSH_OFFSET(nameMask);
    PUSH_OFFSET(nameStore);
    PUSH_OFFSET(nameString);
    PUSH_OFFSET(nextObjectOffset);
    PUSH_OFFSET(partyLeader);
    PUSH_OFFSET(partyPlayer1);
    PUSH_OFFSET(partyPlayer2);
    PUSH_OFFSET(partyPlayer3);
    PUSH_OFFSET(partyPlayer4);
    PUSH_OFFSET(petGUID);
    PUSH_OFFSET(playerBase);
    PUSH_OFFSET(playerCorpseX);
    PUSH_OFFSET(playerCorpseY);
    PUSH_OFFSET(playerCorpseZ);
    PUSH_OFFSET(playerHealth);
    PUSH_OFFSET(playerIsIngame);
    PUSH_OFFSET(playerIsLoadingscreen);
    PUSH_OFFSET(playerName);
    PUSH_OFFSET(realmName);
    PUSH_OFFSET(sendMovementPacket);
    PUSH_OFFSET(setFacing);
    PUSH_OFFSET(staticCastingstate);
    PUSH_OFFSET(subZoneText);
    PUSH_OFFSET(tickCount);
    PUSH_OFFSET(timestamp);
    PUSH_OFFSET(worldLoaded);
    PUSH_OFFSET(wowChat);
    PUSH_OFFSET(wowChatNextMsg);
    PUSH_OFFSET(zoneID);
    PUSH_OFFSET(zoneText);
    PUSH_OFFSET(zoneNamePointer);

#undef PUSH_OFFSET

    lua_setglobal(L, "wow_offsets");

    fprintf(stdout, "[WoTLKLayer] Lua globals 'wow' and 'wow_offsets' registered\n");
}
#endif

VkSemaphore OverlayUI::Render(VkQueue                         queue,
                               uint32_t                        imageIndex,
                               const std::vector<VkSemaphore>& waitSems)
{
    if (!m_ready || !m_context || imageIndex >= m_overlaySemaphores.size())
        return VK_NULL_HANDLE;

    VkFence     fence     = m_fences[imageIndex];
    VkSemaphore signalSem = m_overlaySemaphores[imageIndex];

    // Wait for the previous use of this image's resources to finish
    m_dispatch->WaitForFences(m_device, 1, &fence, VK_TRUE, UINT64_MAX);
    m_dispatch->ResetFences(m_device, 1, &fence);

    // Update RmlUi (runs layout, animations, etc.)
    UpdateGameData();
    m_context->Update();

    // Prepare the Vulkan command buffer for this image
    m_renderer->SetCurrentImageIndex(imageIndex);
    VkCommandBuffer cmd = m_renderer->PrepareFrame();
    if (!cmd) return VK_NULL_HANDLE;

    // Render all RmlUi elements (calls CompileGeometry / RenderGeometry via our interface)
    m_context->Render();

    // End the render pass, end the command buffer, and submit with semaphore chain
    m_renderer->SubmitFrame(queue, waitSems, signalSem, fence);

    return signalSem;
}

// ============================================================================
// Sync object management
// ============================================================================

VkResult OverlayUI::CreateSyncObjects(uint32_t imageCount)
{
    m_overlaySemaphores.resize(imageCount, VK_NULL_HANDLE);
    m_fences.resize(imageCount, VK_NULL_HANDLE);

    VkSemaphoreCreateInfo sci{};
    sci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fci{};
    fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fci.flags = VK_FENCE_CREATE_SIGNALED_BIT;  // Start signaled so first frame doesn't wait

    for (uint32_t i = 0; i < imageCount; ++i) {
        VkResult r = m_dispatch->CreateSemaphore(m_device, &sci, nullptr,
                                                 &m_overlaySemaphores[i]);
        if (r != VK_SUCCESS) return r;

        r = m_dispatch->CreateFence(m_device, &fci, nullptr, &m_fences[i]);
        if (r != VK_SUCCESS) return r;
    }
    return VK_SUCCESS;
}

void OverlayUI::DestroySyncObjects()
{
    for (auto sem : m_overlaySemaphores)
        if (sem) m_dispatch->DestroySemaphore(m_device, sem, nullptr);
    m_overlaySemaphores.clear();

    for (auto fence : m_fences)
        if (fence) m_dispatch->DestroyFence(m_device, fence, nullptr);
    m_fences.clear();
}

} // namespace WoTLKGuiLayer
