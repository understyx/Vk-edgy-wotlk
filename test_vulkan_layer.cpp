#include "VkModernUILayer.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <cassert>
#include <vector>
#include <atomic>
#include <cstring>

// ============================================================================
// 1. CONCURRENT STRESS TEST FOR POINTER-SWAP DOUBLE BUFFERING
// ============================================================================
// Spawns concurrent producer and consumer threads (strictly SPSC per pipeline stage)
// to verify that the PointerSwapBuffer never has race conditions, torn reads, or deadlocks.
void RunPointerSwapStressTest() {
    std::cout << "\n==================================================" << std::endl;
    std::cout << "RUNNING SPSC POINTER-SWAP CONCURRENT STRESS TEST..." << std::endl;
    std::cout << "==================================================" << std::endl;

    PointerSwapBuffer<GameData> stress_buffer;
    std::atomic<bool> run_stress(true);
    std::atomic<uint64_t> total_writes(0);
    std::atomic<uint64_t> total_reads(0);

    // Spawn 1 producer thread (SPSC: Single Producer)
    std::thread producer([&stress_buffer, &run_stress, &total_writes]() {
        uint32_t local_frame = 0;
        while (run_stress.load(std::memory_order_relaxed)) {
            GameData* p = stress_buffer.get_producer_buffer();

            // Write completely coherent data (uncontested)
            uint32_t frame = ++local_frame;
            p->frame_id = frame;
            p->player_x = static_cast<float>(1000 + frame);
            p->player_y = static_cast<float>(2000 + frame);
            p->player_z = static_cast<float>(3000 + frame);
            p->player_hp = 100 - (frame % 50);
            p->player_mp = 100 - (frame % 80);
            std::snprintf(p->target_name, sizeof(p->target_name), "Producer - Frame %u", frame);
            p->timestamp = total_writes.fetch_add(1) + 1;

            stress_buffer.publish();
            std::this_thread::yield();
        }
    });

    // Spawn 1 consumer thread (SPSC: Single Consumer)
    std::thread consumer([&stress_buffer, &run_stress, &total_reads]() {
        while (run_stress.load(std::memory_order_relaxed)) {
            if (stress_buffer.acquire_latest()) {
                GameData* c = stress_buffer.get_consumer_buffer();

                // Verify data coherency - fields must match each other and target_name
                // No "torn reads" should ever occur since writing and reading are decoupled
                uint32_t frame = c->frame_id;
                unsigned int frame_parsed = 0;
                if (std::sscanf(c->target_name, "Producer - Frame %u", &frame_parsed) == 1) {
                    if (frame_parsed != frame) {
                        std::cerr << "MISMATCH DETECTED!" << std::endl;
                        std::cerr << "c->frame_id: " << frame << std::endl;
                        std::cerr << "c->target_name: " << c->target_name << std::endl;
                        std::cerr << "frame_parsed: " << frame_parsed << std::endl;
                        std::cerr << "c->player_x: " << c->player_x << std::endl;
                        std::cerr << "c->player_y: " << c->player_y << std::endl;
                        std::cerr << "c->player_z: " << c->player_z << std::endl;
                        std::cerr << "c->player_hp: " << c->player_hp << std::endl;
                        std::cerr << "c->player_mp: " << c->player_mp << std::endl;
                        std::cerr << "c->timestamp: " << c->timestamp << std::endl;
                        assert(false);
                    }
                    assert(frame_parsed == frame);
                    assert(c->player_x == static_cast<float>(1000 + frame));
                    assert(c->player_y == static_cast<float>(2000 + frame));
                    assert(c->player_z == static_cast<float>(3000 + frame));
                    assert(c->player_hp == 100 - (frame % 50));
                    assert(c->player_mp == 100 - (frame % 80));
                }
                total_reads.fetch_add(1);
            }
            std::this_thread::yield();
        }
    });

    // Let the stress test run for 1 second
    std::this_thread::sleep_for(std::chrono::seconds(1));
    run_stress.store(false);

    producer.join();
    consumer.join();

    std::cout << "SUCCESS: Stress test finished without any race conditions or data corruption!" << std::endl;
    std::cout << "Total concurrent published frames: " << total_writes.load() << std::endl;
    std::cout << "Total concurrent acquired frames: " << total_reads.load() << std::endl;
    std::cout << "==================================================\n" << std::endl;
}

// ============================================================================
// 2. MOCK VULKAN IMPLEMENTATIONS FOR THE DISPATCH CHAIN
// ============================================================================
// To simulate standard Vulkan layer chaining, we define the next functions down the chain.

static VkResult VKAPI_CALL Mock_CreateInstance(
    const VkInstanceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkInstance* pInstance)
{
    std::cout << "  [Mock Vulkan Loader/Driver] CreateInstance executed." << std::endl;
    // Return a dummy pointer as instance
    *pInstance = (VkInstance)0xDEADBEEF1234;
    return VK_SUCCESS;
}

static void VKAPI_CALL Mock_DestroyInstance(
    VkInstance instance,
    const VkAllocationCallbacks* pAllocator)
{
    std::cout << "  [Mock Vulkan Loader/Driver] DestroyInstance executed for " << instance << std::endl;
}

static VkResult VKAPI_CALL Mock_CreateDevice(
    VkPhysicalDevice physicalDevice,
    const VkDeviceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDevice* pDevice)
{
    std::cout << "  [Mock Vulkan Loader/Driver] CreateDevice executed." << std::endl;
    // Return a dummy pointer as device
    *pDevice = (VkDevice)0xBEEFDEAD5678;
    return VK_SUCCESS;
}

static void VKAPI_CALL Mock_DestroyDevice(
    VkDevice device,
    const VkAllocationCallbacks* pAllocator)
{
    std::cout << "  [Mock Vulkan Loader/Driver] DestroyDevice executed for " << device << std::endl;
}

static VkResult VKAPI_CALL Mock_QueuePresentKHR(
    VkQueue queue,
    const VkPresentInfoKHR* pPresentInfo)
{
    std::cout << "  [Mock Vulkan Loader/Driver] QueuePresentKHR executed. Frame presented!" << std::endl;
    return VK_SUCCESS;
}

// Mock GetInstanceProcAddr for the next layer in the chain
static PFN_vkVoidFunction VKAPI_CALL Mock_GetInstanceProcAddr(VkInstance instance, const char* pName) {
    if (std::strcmp(pName, "vkCreateInstance") == 0) return (PFN_vkVoidFunction)Mock_CreateInstance;
    if (std::strcmp(pName, "vkDestroyInstance") == 0) return (PFN_vkVoidFunction)Mock_DestroyInstance;
    if (std::strcmp(pName, "vkCreateDevice") == 0) return (PFN_vkVoidFunction)Mock_CreateDevice;
    return nullptr;
}

// Mock GetDeviceProcAddr for the next layer in the chain
static PFN_vkVoidFunction VKAPI_CALL Mock_GetDeviceProcAddr(VkDevice device, const char* pName) {
    if (std::strcmp(pName, "vkDestroyDevice") == 0) return (PFN_vkVoidFunction)Mock_DestroyDevice;
    if (std::strcmp(pName, "vkQueuePresentKHR") == 0) return (PFN_vkVoidFunction)Mock_QueuePresentKHR;
    return nullptr;
}

// ============================================================================
// 3. VULKAN LAYER INTEGRATION TEST
// ============================================================================
void RunVulkanLayerIntegrationTest() {
    std::cout << "==================================================" << std::endl;
    std::cout << "RUNNING VULKAN LAYER INTEGRATION TEST..." << std::endl;
    std::cout << "==================================================" << std::endl;

    // 1. Get layer version negotiation function and verify compatibility
    VkNegotiateLayerInterface version_struct;
    std::memset(&version_struct, 0, sizeof(version_struct));
    version_struct.sType = LAYER_NEGOTIATE_INTERFACE_STRUCT;
    version_struct.loaderLayerInterfaceVersion = 2;

    VkResult res = vkNegotiateLoaderLayerInterfaceVersion(&version_struct);
    assert(res == VK_SUCCESS);
    assert(version_struct.pfnGetInstanceProcAddr != nullptr);
    assert(version_struct.pfnGetDeviceProcAddr != nullptr);

    std::cout << "SUCCESS: Vulkan loader interface negotiation passed!" << std::endl;

    // 2. Simulate vkCreateInstance with standard loader link chain structures
    VkLayerInstanceLink instance_link;
    instance_link.pNext = nullptr;
    instance_link.pfnNextGetInstanceProcAddr = Mock_GetInstanceProcAddr;
    instance_link.pfnNextGetPhysicalDeviceProcAddr = nullptr;

    VkLayerInstanceCreateInfo instance_layer_info;
    std::memset(&instance_layer_info, 0, sizeof(instance_layer_info));
    instance_layer_info.sType = (VkStructureType)VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO;
    instance_layer_info.pNext = nullptr;
    instance_layer_info.function = VK_LAYER_LINK_INFO;
    instance_layer_info.u.pLayerInfo = &instance_link;

    VkInstanceCreateInfo instance_create_info;
    std::memset(&instance_create_info, 0, sizeof(instance_create_info));
    instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_create_info.pNext = &instance_layer_info;

    VkInstance instance = VK_NULL_HANDLE;
    // Resolve Hook_CreateInstance via layer GIPA
    PFN_vkCreateInstance CreateInstanceHook = (PFN_vkCreateInstance)version_struct.pfnGetInstanceProcAddr(VK_NULL_HANDLE, "vkCreateInstance");
    assert(CreateInstanceHook != nullptr);

    std::cout << "\nCreating Vulkan instance (will spawn layer threads)..." << std::endl;
    res = CreateInstanceHook(&instance_create_info, nullptr, &instance);
    assert(res == VK_SUCCESS);
    assert(instance != VK_NULL_HANDLE);
    assert(AreThreadsRunning() == true);

    std::cout << "SUCCESS: Vulkan instance and background threads created!" << std::endl;

    // 3. Simulate vkCreateDevice with standard loader link chain structures
    VkLayerDeviceLink device_link;
    device_link.pNext = nullptr;
    device_link.pfnNextGetInstanceProcAddr = Mock_GetInstanceProcAddr;
    device_link.pfnNextGetDeviceProcAddr = Mock_GetDeviceProcAddr;

    VkLayerDeviceCreateInfo device_layer_info;
    std::memset(&device_layer_info, 0, sizeof(device_layer_info));
    device_layer_info.sType = (VkStructureType)VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO;
    device_layer_info.pNext = nullptr;
    device_layer_info.function = VK_LAYER_LINK_INFO;
    device_layer_info.u.pLayerInfo = &device_link;

    VkDeviceCreateInfo device_create_info;
    std::memset(&device_create_info, 0, sizeof(device_create_info));
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.pNext = &device_layer_info;

    VkDevice device = VK_NULL_HANDLE;
    // Resolve Hook_CreateDevice via layer GIPA
    PFN_vkCreateDevice CreateDeviceHook = (PFN_vkCreateDevice)version_struct.pfnGetInstanceProcAddr(instance, "vkCreateDevice");
    assert(CreateDeviceHook != nullptr);

    std::cout << "\nCreating Vulkan device (will register dispatch tables)..." << std::endl;
    VkPhysicalDevice physical_device = (VkPhysicalDevice)0x55554444; // Dummy physical device
    res = CreateDeviceHook(physical_device, &device_create_info, nullptr, &device);
    assert(res == VK_SUCCESS);
    assert(device != VK_NULL_HANDLE);

    std::cout << "SUCCESS: Vulkan device created and dispatch registered!" << std::endl;

    // 4. Simulate a Game frame-presentation render loop calling vkQueuePresentKHR
    PFN_vkQueuePresentKHR QueuePresentHook = (PFN_vkQueuePresentKHR)version_struct.pfnGetDeviceProcAddr(device, "vkQueuePresentKHR");
    assert(QueuePresentHook != nullptr);

    VkQueue queue = (VkQueue)0x11112222; // Dummy presentation queue
    VkPresentInfoKHR present_info;
    std::memset(&present_info, 0, sizeof(present_info));
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    std::cout << "\nSimulating game frame rendering loop with UI overlay presenting..." << std::endl;
    for (int frame = 1; frame <= 5; ++frame) {
        std::cout << "\n--- GAME RENDERING FRAME " << frame << " ---" << std::endl;
        res = QueuePresentHook(queue, &present_info);
        assert(res == VK_SUCCESS);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // 5. Clean up device and instance
    PFN_vkDestroyDevice DestroyDeviceHook = (PFN_vkDestroyDevice)version_struct.pfnGetDeviceProcAddr(device, "vkDestroyDevice");
    assert(DestroyDeviceHook != nullptr);

    std::cout << "\nDestroying Vulkan device..." << std::endl;
    DestroyDeviceHook(device, nullptr);

    PFN_vkDestroyInstance DestroyInstanceHook = (PFN_vkDestroyInstance)version_struct.pfnGetInstanceProcAddr(instance, "vkDestroyInstance");
    assert(DestroyInstanceHook != nullptr);

    std::cout << "Destroying Vulkan instance (will terminate background threads)..." << std::endl;
    DestroyInstanceHook(instance, nullptr);
    assert(AreThreadsRunning() == false);

    std::cout << "\n==================================================" << std::endl;
    std::cout << "ALL VULKAN LAYER INTEGRATION TESTS PASSED SUCCESS!" << std::endl;
    std::cout << "==================================================" << std::endl;
}

// ============================================================================
// MAIN ENTRY POINT
// ============================================================================
int main() {
    RunPointerSwapStressTest();
    RunVulkanLayerIntegrationTest();
    return 0;
}
