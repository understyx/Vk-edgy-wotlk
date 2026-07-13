#include "VkModernUILayer.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <cmath>
#include <unordered_map>
#include <mutex>
#include <atomic>

// ============================================================================
// GLOBAL BUFFERS DEFINITION (STRICT SPSC PIPELINE SEGMENTS)
// ============================================================================
PointerSwapBuffer<GameData> g_game_data_pub;
PointerSwapBuffer<UIRenderData> g_ui_render_pub;
PointerSwapBuffer<VulkanDrawState> g_vk_draw_state_pub;

// ============================================================================
// THREAD SYSTEM VARIABLES
// ============================================================================
static std::thread g_game_data_thread;
static std::thread g_js_html_thread;
static std::thread g_vulkan_render_thread;

static std::atomic<bool> g_threads_active(false);
static std::mutex g_thread_control_mutex;

// ============================================================================
// 1. GAME DATA READ THREAD (THREAD 1 - STUB)
// ============================================================================
// Simulates a background thread inside/interfacing with the old video game
// engine, reading internal game metrics/variables and publishing them.
static void GameDataReadThreadFunc() {
    std::cout << "[Game Data Thread] Thread started." << std::endl;
    uint32_t simulated_frame_id = 0;
    float angle = 0.0f;

    while (g_threads_active.load(std::memory_order_acquire)) {
        // 1. Get the private producer buffer
        GameData* data = g_game_data_pub.get_producer_buffer();

        // 2. Simulate reading game coordinates, health, and target info
        simulated_frame_id++;
        angle += 0.05f;
        if (angle > 2.0f * M_PI) angle -= 2.0f * M_PI;

        data->frame_id = simulated_frame_id;
        data->player_x = std::cos(angle) * 150.0f;
        data->player_y = std::sin(angle) * 150.0f;
        data->player_z = 10.0f + std::sin(angle * 2.0f) * 2.0f;

        // Simulate HP and MP draining/regaining
        data->player_hp = 50 + static_cast<uint32_t>((std::sin(angle) + 1.0f) * 25.0f);
        data->player_mp = 80 + static_cast<uint32_t>((std::cos(angle) + 1.0f) * 10.0f);

        if (simulated_frame_id % 150 < 50) {
            std::strncpy(data->target_name, "Lich King", sizeof(data->target_name));
        } else if (simulated_frame_id % 150 < 100) {
            std::strncpy(data->target_name, "Illidan Stormrage", sizeof(data->target_name));
        } else {
            std::strncpy(data->target_name, "None", sizeof(data->target_name));
        }

        data->timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count();

        // 3. Publish the new game data to the shared buffer
        g_game_data_pub.publish();

        std::cout << "[Game Data Thread] Published Frame " << data->frame_id
                  << " | Player HP: " << data->player_hp << "% | Target: " << data->target_name << std::endl;

        // Wait a short time (e.g. 50ms = 20Hz update rate)
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    std::cout << "[Game Data Thread] Thread stopped cleanly." << std::endl;
}

// ============================================================================
// 2. JS / HTML ENGINE THREAD (THREAD 2)
// ============================================================================
// Simulates running a JS execution context (V8, QuickJS) and HTML layout engine
// to compute modern UI placements/styles based on the game's state.
static void JSHTMLEngineThreadFunc() {
    std::cout << "[JS/HTML Engine Thread] Thread started." << std::endl;
    uint32_t simulated_render_frame = 0;

    while (g_threads_active.load(std::memory_order_acquire)) {
        // 1. Retrieve the latest game data
        bool new_game_data = g_game_data_pub.acquire_latest();
        GameData* game = g_game_data_pub.get_consumer_buffer();

        // 2. Run Simulated JS/HTML Layout Engine & generate modern UI overlay
        simulated_render_frame++;
        UIRenderData* ui = g_ui_render_pub.get_producer_buffer();
        ui->frame_id = simulated_render_frame;
        ui->render_timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count();

        // Layout generated elements (e.g. Health Bar, Mana Bar, Target Info window)
        uint32_t idx = 0;

        // Draw HUD container
        std::strncpy(ui->elements[idx].tag, "div-hud", sizeof(ui->elements[idx].tag));
        ui->elements[idx].x = 10.0f;
        ui->elements[idx].y = 10.0f;
        ui->elements[idx].width = 300.0f;
        ui->elements[idx].height = 120.0f;
        ui->elements[idx].color = 0x222222DD; // Dark semi-transparent
        std::strncpy(ui->elements[idx].text, "Modern UI Hud Container", sizeof(ui->elements[idx].text));
        idx++;

        // Draw modern health bar overlay (dynamic width based on HP)
        std::strncpy(ui->elements[idx].tag, "div-hpbar", sizeof(ui->elements[idx].tag));
        ui->elements[idx].x = 20.0f;
        ui->elements[idx].y = 40.0f;
        ui->elements[idx].width = (static_cast<float>(game->player_hp) / 100.0f) * 280.0f;
        ui->elements[idx].height = 20.0f;
        ui->elements[idx].color = 0x00FF00FF; // Solid Green
        std::snprintf(ui->elements[idx].text, sizeof(ui->elements[idx].text), "HP: %u%%", game->player_hp);
        idx++;

        // Draw modern mana bar overlay (dynamic width based on MP)
        std::strncpy(ui->elements[idx].tag, "div-mpbar", sizeof(ui->elements[idx].tag));
        ui->elements[idx].x = 20.0f;
        ui->elements[idx].y = 70.0f;
        ui->elements[idx].width = (static_cast<float>(game->player_mp) / 100.0f) * 280.0f;
        ui->elements[idx].height = 15.0f;
        ui->elements[idx].color = 0x0000FFFF; // Solid Blue
        std::snprintf(ui->elements[idx].text, sizeof(ui->elements[idx].text), "MP: %u%%", game->player_mp);
        idx++;

        // Draw Player Coordinates display
        std::strncpy(ui->elements[idx].tag, "p-coords", sizeof(ui->elements[idx].tag));
        ui->elements[idx].x = 20.0f;
        ui->elements[idx].y = 95.0f;
        ui->elements[idx].width = 280.0f;
        ui->elements[idx].height = 15.0f;
        ui->elements[idx].color = 0xFFFFFFFF; // Solid White text
        std::snprintf(ui->elements[idx].text, sizeof(ui->elements[idx].text),
                     "XYZ: (%.2f, %.2f, %.2f)", game->player_x, game->player_y, game->player_z);
        idx++;

        // Draw Active Target Info Box
        if (std::strcmp(game->target_name, "None") != 0) {
            std::strncpy(ui->elements[idx].tag, "div-target", sizeof(ui->elements[idx].tag));
            ui->elements[idx].x = 350.0f;
            ui->elements[idx].y = 10.0f;
            ui->elements[idx].width = 200.0f;
            ui->elements[idx].height = 40.0f;
            ui->elements[idx].color = 0xFF0000AA; // Red semi-transparent
            std::snprintf(ui->elements[idx].text, sizeof(ui->elements[idx].text), "Target: %s", game->target_name);
            idx++;
        }

        ui->element_count = idx;

        // 3. Publish layout results to Vulkan rendering thread
        g_ui_render_pub.publish();

        std::cout << "[JS/HTML Engine Thread] Processed game data frame " << game->frame_id
                  << (new_game_data ? " (NEW)" : " (CACHED)")
                  << " -> Compiled modern UI frame " << ui->frame_id
                  << " with " << ui->element_count << " UI elements" << std::endl;

        // Run at ~30Hz (33ms sleep)
        std::this_thread::sleep_for(std::chrono::milliseconds(33));
    }
    std::cout << "[JS/HTML Engine Thread] Thread stopped cleanly." << std::endl;
}

// ============================================================================
// 3. VULKAN RENDERING / LAYER THREAD (THREAD 3)
// ============================================================================
// Simulates a background thread or direct presentation-layer thread that consumes
// modern HTML/JS output frames, doing the heavy Vulkan render command recording.
// After compilation, it publishes the drawable Vulkan draw states to g_vk_draw_state_pub.
static void VulkanRenderingThreadFunc() {
    std::cout << "[Vulkan Render Thread] Thread started." << std::endl;
    while (g_threads_active.load(std::memory_order_acquire)) {
        // 1. Retrieve the latest UI layout
        bool new_ui_frame = g_ui_render_pub.acquire_latest();
        UIRenderData* ui = g_ui_render_pub.get_consumer_buffer();

        // 2. Perform Vulkan draw state construction (record commands/build draw calls)
        VulkanDrawState* draw_state = g_vk_draw_state_pub.get_producer_buffer();
        draw_state->ui_frame_id = ui->frame_id;
        draw_state->draw_call_count = ui->element_count;
        draw_state->compiled_timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count();

        std::cout << "[Vulkan Render Thread] "
                  << (new_ui_frame ? "Acquired NEW" : "Drawing cached")
                  << " UI Frame " << ui->frame_id
                  << " | Recording heavy Vulkan command buffers..." << std::endl;

        for (uint32_t i = 0; i < ui->element_count; ++i) {
            auto& el = ui->elements[i];
            auto& dc = draw_state->draw_calls[i];

            std::strncpy(dc.element_tag, el.tag, sizeof(dc.element_tag));
            dc.rect_x = el.x;
            dc.rect_y = el.y;
            dc.rect_w = el.width;
            dc.rect_h = el.height;
            dc.color = el.color;
            std::strncpy(dc.text_content, el.text, sizeof(dc.text_content));

            std::cout << "    -> [VK Command Record " << i << "]: " << dc.element_tag
                      << " [Pos: " << dc.rect_x << "," << dc.rect_y << " Size: " << dc.rect_w << "x" << dc.rect_h
                      << " Color: 0x" << std::hex << dc.color << std::dec << " Text: \"" << dc.text_content << "\"]" << std::endl;
        }

        // 3. Publish the recorded Vulkan commands/states to the presentation thread
        g_vk_draw_state_pub.publish();

        // Run at ~20Hz for simulation logging (prevent logs from moving too fast)
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    std::cout << "[Vulkan Render Thread] Thread stopped cleanly." << std::endl;
}

// ============================================================================
// 4. BACKGROUND THREAD CONTROL
// ============================================================================
void StartModernUIThreads() {
    std::lock_guard<std::mutex> lock(g_thread_control_mutex);
    if (g_threads_active.load(std::memory_order_acquire)) {
        return; // Already running
    }
    std::cout << "\n[Vulkan Layer initialization] Spawning 3 concurrent threads for modernizing UI...\n";
    g_threads_active.store(true, std::memory_order_release);

    g_game_data_thread = std::thread(GameDataReadThreadFunc);
    g_js_html_thread = std::thread(JSHTMLEngineThreadFunc);
    g_vulkan_render_thread = std::thread(VulkanRenderingThreadFunc);
}

void StopModernUIThreads() {
    std::lock_guard<std::mutex> lock(g_thread_control_mutex);
    if (!g_threads_active.load(std::memory_order_acquire)) {
        return; // Already stopped
    }
    std::cout << "\n[Vulkan Layer destruction] Shutting down concurrent UI modernisation threads...\n";
    g_threads_active.store(false, std::memory_order_release);

    if (g_game_data_thread.joinable()) {
        g_game_data_thread.join();
    }
    if (g_js_html_thread.joinable()) {
        g_js_html_thread.join();
    }
    if (g_vulkan_render_thread.joinable()) {
        g_vulkan_render_thread.join();
    }
    std::cout << "[Vulkan Layer destruction] All threads shut down successfully.\n\n";
}

bool AreThreadsRunning() {
    return g_threads_active.load(std::memory_order_acquire);
}

// ============================================================================
// 5. VULKAN LAYER DISPATCH & INTERCEPTION IMPLEMENTATION
// ============================================================================

struct InstanceDispatch {
    PFN_vkGetInstanceProcAddr nextGetInstanceProcAddr = nullptr;
    PFN_vkDestroyInstance nextDestroyInstance = nullptr;
};

struct DeviceDispatch {
    PFN_vkGetDeviceProcAddr nextGetDeviceProcAddr = nullptr;
    PFN_vkQueuePresentKHR nextQueuePresentKHR = nullptr;
    PFN_vkDestroyDevice nextDestroyDevice = nullptr;
};

static std::unordered_map<VkInstance, InstanceDispatch> g_instance_dispatch;
static std::unordered_map<VkDevice, DeviceDispatch> g_device_dispatch;
static std::mutex g_dispatch_mutex;

// --- Instance Hooks ---

VK_LAYER_EXPORT VkResult VKAPI_CALL Hook_CreateInstance(
    const VkInstanceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkInstance* pInstance)
{
    std::cout << "[Vulkan Layer Hook] Hook_CreateInstance called." << std::endl;

    // Locate the loader instance create info struct containing the link chain
    VkLayerInstanceCreateInfo* layerCreateInfo = (VkLayerInstanceCreateInfo*)pCreateInfo->pNext;
    while (layerCreateInfo &&
           !(layerCreateInfo->sType == VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO &&
             layerCreateInfo->function == VK_LAYER_LINK_INFO)) {
        layerCreateInfo = (VkLayerInstanceCreateInfo*)layerCreateInfo->pNext;
    }

    if (layerCreateInfo == nullptr) {
        std::cerr << "[Vulkan Layer Hook] Error: Failed to find VK_LAYER_LINK_INFO in CreateInstance chain." << std::endl;
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    // Extract nextGetInstanceProcAddr and step forward in the chain
    PFN_vkGetInstanceProcAddr nextGetInstanceProcAddr = layerCreateInfo->u.pLayerInfo->pfnNextGetInstanceProcAddr;
    layerCreateInfo->u.pLayerInfo = layerCreateInfo->u.pLayerInfo->pNext;

    // Load actual vkCreateInstance down the chain
    PFN_vkCreateInstance nextCreateInstance = (PFN_vkCreateInstance)nextGetInstanceProcAddr(VK_NULL_HANDLE, "vkCreateInstance");
    if (!nextCreateInstance) {
        std::cerr << "[Vulkan Layer Hook] Error: Failed to resolve vkCreateInstance from down-chain GPA." << std::endl;
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    // Call actual Vulkan create instance
    VkResult result = nextCreateInstance(pCreateInfo, pAllocator, pInstance);
    if (result != VK_SUCCESS) {
        std::cerr << "[Vulkan Layer Hook] Error: vkCreateInstance returned " << result << std::endl;
        return result;
    }

    // Save instance dispatch details
    {
        std::lock_guard<std::mutex> lock(g_dispatch_mutex);
        InstanceDispatch disp;
        disp.nextGetInstanceProcAddr = nextGetInstanceProcAddr;
        disp.nextDestroyInstance = (PFN_vkDestroyInstance)nextGetInstanceProcAddr(*pInstance, "vkDestroyInstance");
        g_instance_dispatch[*pInstance] = disp;
    }

    // Start background modernisation threads
    StartModernUIThreads();

    return VK_SUCCESS;
}

VK_LAYER_EXPORT void VKAPI_CALL Hook_DestroyInstance(
    VkInstance instance,
    const VkAllocationCallbacks* pAllocator)
{
    std::cout << "[Vulkan Layer Hook] Hook_DestroyInstance called." << std::endl;
    PFN_vkDestroyInstance nextDestroyInstance = nullptr;
    {
        std::lock_guard<std::mutex> lock(g_dispatch_mutex);
        auto it = g_instance_dispatch.find(instance);
        if (it != g_instance_dispatch.end()) {
            nextDestroyInstance = it->second.nextDestroyInstance;
            g_instance_dispatch.erase(it);
        }
    }

    // Terminate UI modernisation threads
    StopModernUIThreads();

    if (nextDestroyInstance) {
        nextDestroyInstance(instance, pAllocator);
    }
}

// --- Device Hooks ---

VK_LAYER_EXPORT VkResult VKAPI_CALL Hook_CreateDevice(
    VkPhysicalDevice physicalDevice,
    const VkDeviceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDevice* pDevice)
{
    std::cout << "[Vulkan Layer Hook] Hook_CreateDevice called." << std::endl;

    // Locate the loader device create info struct containing the link chain
    VkLayerDeviceCreateInfo* layerCreateInfo = (VkLayerDeviceCreateInfo*)pCreateInfo->pNext;
    while (layerCreateInfo &&
           !(layerCreateInfo->sType == VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO &&
             layerCreateInfo->function == VK_LAYER_LINK_INFO)) {
        layerCreateInfo = (VkLayerDeviceCreateInfo*)layerCreateInfo->pNext;
    }

    if (layerCreateInfo == nullptr) {
        std::cerr << "[Vulkan Layer Hook] Error: Failed to find VK_LAYER_LINK_INFO in CreateDevice chain." << std::endl;
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    // Extract nextGetDeviceProcAddr and step forward in the chain
    PFN_vkGetDeviceProcAddr nextGetDeviceProcAddr = layerCreateInfo->u.pLayerInfo->pfnNextGetDeviceProcAddr;
    layerCreateInfo->u.pLayerInfo = layerCreateInfo->u.pLayerInfo->pNext;

    // Resolve nextCreateDevice using nextGetInstanceProcAddr from instance
    PFN_vkGetInstanceProcAddr nextGIPA = nullptr;
    {
        std::lock_guard<std::mutex> lock(g_dispatch_mutex);
        if (!g_instance_dispatch.empty()) {
            nextGIPA = g_instance_dispatch.begin()->second.nextGetInstanceProcAddr;
        }
    }

    if (!nextGIPA) {
        std::cerr << "[Vulkan Layer Hook] Error: No active instance dispatch table entries to resolve CreateDevice." << std::endl;
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    PFN_vkCreateDevice nextCreateDevice = (PFN_vkCreateDevice)nextGIPA(VK_NULL_HANDLE, "vkCreateDevice");
    if (!nextCreateDevice) {
        std::cerr << "[Vulkan Layer Hook] Error: Failed to resolve vkCreateDevice." << std::endl;
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    // Call actual Vulkan create device
    VkResult result = nextCreateDevice(physicalDevice, pCreateInfo, pAllocator, pDevice);
    if (result != VK_SUCCESS) {
        std::cerr << "[Vulkan Layer Hook] Error: vkCreateDevice returned " << result << std::endl;
        return result;
    }

    // Save device dispatch details
    {
        std::lock_guard<std::mutex> lock(g_dispatch_mutex);
        DeviceDispatch disp;
        disp.nextGetDeviceProcAddr = nextGetDeviceProcAddr;
        disp.nextQueuePresentKHR = (PFN_vkQueuePresentKHR)nextGetDeviceProcAddr(*pDevice, "vkQueuePresentKHR");
        disp.nextDestroyDevice = (PFN_vkDestroyDevice)nextGetDeviceProcAddr(*pDevice, "vkDestroyDevice");
        g_device_dispatch[*pDevice] = disp;
    }

    return VK_SUCCESS;
}

VK_LAYER_EXPORT void VKAPI_CALL Hook_DestroyDevice(
    VkDevice device,
    const VkAllocationCallbacks* pAllocator)
{
    std::cout << "[Vulkan Layer Hook] Hook_DestroyDevice called." << std::endl;
    PFN_vkDestroyDevice nextDestroyDevice = nullptr;
    {
        std::lock_guard<std::mutex> lock(g_dispatch_mutex);
        auto it = g_device_dispatch.find(device);
        if (it != g_device_dispatch.end()) {
            nextDestroyDevice = it->second.nextDestroyDevice;
            g_device_dispatch.erase(it);
        }
    }

    if (nextDestroyDevice) {
        nextDestroyDevice(device, pAllocator);
    }
}

VK_LAYER_EXPORT VkResult VKAPI_CALL Hook_QueuePresentKHR(
    VkQueue queue,
    const VkPresentInfoKHR* pPresentInfo)
{
    // Retrieve nextQueuePresentKHR from dispatch table
    PFN_vkQueuePresentKHR nextQueuePresentKHR = nullptr;
    {
        std::lock_guard<std::mutex> lock(g_dispatch_mutex);
        if (!g_device_dispatch.empty()) {
            nextQueuePresentKHR = g_device_dispatch.begin()->second.nextQueuePresentKHR;
        }
    }

    // Inside the Game's present/frame-end hook, retrieve the compiled VulkanDrawState to present.
    // Hook_QueuePresentKHR is the ONLY consumer of g_vk_draw_state_pub, adhering to strict SPSC.
    bool is_new_draw = g_vk_draw_state_pub.acquire_latest();
    VulkanDrawState* draw_state = g_vk_draw_state_pub.get_consumer_buffer();

    std::cout << "[Vulkan Layer - QueuePresent Hook] Presenting UI overlay frame: " << draw_state->ui_frame_id
              << (is_new_draw ? " (FRESH DRAW STATE ACQUIRED)" : " (RE-USED PREVIOUS DRAW STATE)")
              << " containing " << draw_state->draw_call_count << " compiled draw calls." << std::endl;

    for (uint32_t i = 0; i < draw_state->draw_call_count; ++i) {
        auto& dc = draw_state->draw_calls[i];
        std::cout << "  -> [Present Executing Overlay Draw]: " << dc.element_tag
                  << " at (" << dc.rect_x << "," << dc.rect_y << ") size " << dc.rect_w << "x" << dc.rect_h
                  << " text: \"" << dc.text_content << "\"" << std::endl;
    }

    if (nextQueuePresentKHR) {
        return nextQueuePresentKHR(queue, pPresentInfo);
    }

    return VK_SUCCESS;
}

// --- Dynamic Function Resolution Hook Entries ---

VK_LAYER_EXPORT PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(VkDevice device, const char* pName) {
    if (std::strcmp(pName, "vkGetDeviceProcAddr") == 0) return (PFN_vkVoidFunction)vkGetDeviceProcAddr;
    if (std::strcmp(pName, "vkDestroyDevice") == 0) return (PFN_vkVoidFunction)Hook_DestroyDevice;
    if (std::strcmp(pName, "vkQueuePresentKHR") == 0) return (PFN_vkVoidFunction)Hook_QueuePresentKHR;

    std::lock_guard<std::mutex> lock(g_dispatch_mutex);
    auto it = g_device_dispatch.find(device);
    if (it != g_device_dispatch.end() && it->second.nextGetDeviceProcAddr) {
        return it->second.nextGetDeviceProcAddr(device, pName);
    }
    return nullptr;
}

VK_LAYER_EXPORT PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance instance, const char* pName) {
    if (std::strcmp(pName, "vkGetInstanceProcAddr") == 0) return (PFN_vkVoidFunction)vkGetInstanceProcAddr;
    if (std::strcmp(pName, "vkCreateInstance") == 0) return (PFN_vkVoidFunction)Hook_CreateInstance;
    if (std::strcmp(pName, "vkDestroyInstance") == 0) return (PFN_vkVoidFunction)Hook_DestroyInstance;
    if (std::strcmp(pName, "vkCreateDevice") == 0) return (PFN_vkVoidFunction)Hook_CreateDevice;

    // Also resolve device functions if instance is valid and name matches device hooks
    if (std::strcmp(pName, "vkGetDeviceProcAddr") == 0) return (PFN_vkVoidFunction)vkGetDeviceProcAddr;
    if (std::strcmp(pName, "vkDestroyDevice") == 0) return (PFN_vkVoidFunction)Hook_DestroyDevice;
    if (std::strcmp(pName, "vkQueuePresentKHR") == 0) return (PFN_vkVoidFunction)Hook_QueuePresentKHR;

    std::lock_guard<std::mutex> lock(g_dispatch_mutex);
    auto it = g_instance_dispatch.find(instance);
    if (it != g_instance_dispatch.end() && it->second.nextGetInstanceProcAddr) {
        return it->second.nextGetInstanceProcAddr(instance, pName);
    }
    return nullptr;
}

VK_LAYER_EXPORT VkResult VKAPI_CALL vkNegotiateLoaderLayerInterfaceVersion(VkNegotiateLayerInterface *pVersionStruct) {
    std::cout << "[Vulkan Layer Hook] vkNegotiateLoaderLayerInterfaceVersion called." << std::endl;
    if (pVersionStruct->sType != LAYER_NEGOTIATE_INTERFACE_STRUCT) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    if (pVersionStruct->loaderLayerInterfaceVersion < MIN_SUPPORTED_LOADER_LAYER_INTERFACE_VERSION) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    if (pVersionStruct->loaderLayerInterfaceVersion > CURRENT_LOADER_LAYER_INTERFACE_VERSION) {
        pVersionStruct->loaderLayerInterfaceVersion = CURRENT_LOADER_LAYER_INTERFACE_VERSION;
    }
    pVersionStruct->pfnGetInstanceProcAddr = vkGetInstanceProcAddr;
    pVersionStruct->pfnGetDeviceProcAddr = vkGetDeviceProcAddr;
    pVersionStruct->pfnGetPhysicalDeviceProcAddr = nullptr; // Optional/unneeded for interface V2
    return VK_SUCCESS;
}
