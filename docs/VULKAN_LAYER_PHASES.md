# Vulkan Layer Phases

This document describes the lifecycle phases of a Vulkan layer and how the overlay is injected into the rendering pipeline.

## Phase 1: Device Initialization

### What Happens
The game initializes Vulkan by creating a logical device from a physical device.

### Vulkan Concepts to Understand
- Physical devices (GPU hardware)
- Logical devices (interface to GPU)
- Queues (command submission points)

### Hook: `vkCreateDevice`

This is where Vulkan creates the logical device. The layer hooks this to:
1. Call the original `vkCreateDevice`
2. Save the returned `VkDevice` handle in `gOverlay.device`

This gives us a handle to interact with the GPU.

```cpp
VkResult VkInstanceOverrides::CreateDevice(
    const vkroots::VkDeviceDispatch& pDispatch, 
    VkPhysicalDevice physicalDevice, 
    const VkDeviceCreateInfo* pCreateInfo, 
    const VkAllocationCallbacks* pAllocator, 
    VkDevice* pDevice) 
{
    VkResult result = pDispatch.CreateDevice(physicalDevice, pCreateInfo, pAllocator, pDevice);
    if (result == VK_SUCCESS) {
        gOverlay.device = *pDevice;  // Save for later use
    }
    return result;
}
```

## Phase 2: Swapchain Creation

### What Happens
The game creates a swapchain - the structure that manages the images to be displayed on screen.

### Vulkan Concepts to Understand
- Swapchains (collection of images for presentation)
- Present modes (how images are presented: FIFO, Mailbox, etc.)
- Surface formats (image format: RGBA, BGRA, etc.)

### Hook: `vkCreateSwapchainKHR`

This hook captures:
1. The swapchain handle
2. Image format (RGBA/BGRA/etc.)
3. Image dimensions (width, height)
4. Number of images in the swapchain

```cpp
if (result == VK_SUCCESS) {
    gOverlay.swapchain = *pSwapchain;
    gOverlay.extent = pCreateInfo->imageExtent;  // Width/height
    gOverlay.format = pCreateInfo->imageFormat;  // Format
}
```

This information is essential for creating our overlay rendering pipeline that must match the game's setup.

## Phase 3: Swapchain Images

### What Happens
The layer retrieves the actual image handles from the swapchain. These are the images that will eventually appear on screen.

### Vulkan Concepts to Understand
- Images (GPU-side textures)
- Image views (views into images)

### Hook: `vkGetSwapchainImagesKHR`

The game queries the swapchain to get the actual image handles. The layer intercepts this to store the images:

```cpp
if (result == VK_SUCCESS && pSwapchainImages) {
    gOverlay.images.assign(
        pSwapchainImages,
        pSwapchainImages + *pSwapchainImageCount);
}
```

Now we have references to all the images that the game will render into.

Example: If the swapchain has 3 images (triple buffering):
- Image 0
- Image 1
- Image 2

These are the actual render targets for each frame.

## Phase 4: Per-Frame - Image Acquisition

### What Happens
Every frame, the game acquires an image from the swapchain to render into.

### Vulkan Concepts to Understand
- Acquire/Present model (how images are cycled)
- Synchronization primitives (semaphores, fences)

### Hook: `vkAcquireNextImageKHR`

Every frame, the game calls this to get the next available image:

```cpp
VkResult ret = vkAcquireNextImageKHR(
    device, swapchain, timeout, 
    semaphore, fence, &imageIndex);
```

Returns something like: "Image 2 is available for rendering"

This tells us which image the application is rendering to, so we know where to inject our overlay.

## Phase 5: Command Recording and Submission

### What Happens
The game records Vulkan commands (draw calls, transfers, etc.) into command buffers, then submits them to queues for GPU execution.

### Vulkan Concepts to Understand
- Command pools and command buffers (recording GPU work)
- Queues (GPU execution points)
- Queue submissions

### Current Implementation

We don't currently create our own overlay render pass. Instead, we log when the game submits commands:

```cpp
static VkResult QueueSubmit(
    const vkroots::VkQueueDispatch& pDispatch, 
    VkQueue queue, 
    uint32_t submitCount, 
    const VkSubmitInfo* pSubmits, 
    VkFence fence)
{
    printf("nothing blew up vkQueueSubmit!\n");
    return pDispatch.QueueSubmit(queue, submitCount, pSubmits, fence);
}
```

### Future: Custom Overlay Rendering

Eventually, we'll:
1. Create our own command buffer in Phase 3
2. Record overlay geometry (rectangles, text, etc.)
3. In this phase, BEFORE the game's submissions, insert a barrier and our commands
4. This renders the overlay into the current image
5. Then let the game submit its main rendering

The sequence would be:
```
vkCmdPipelineBarrier()           // Transition image layout if needed
vkCmdBeginRendering()            // Start rendering
vkCmdBindPipeline()              // Bind overlay pipeline
vkCmdBindVertexBuffers()         // Bind overlay geometry
vkCmdDraw()                       // Draw overlay
vkCmdEndRendering()              // Finish rendering
[Game's submissions here]
```

## Phase 6: Frame Presentation

### What Happens
The game presents the finished frame to the display.

### Vulkan Concepts to Understand
- Presentation (display update)
- Image layouts (how the GPU expects the image to be configured)

### Hook: `vkQueuePresentKHR`

This is the final step of the frame:

```cpp
static VkResult QueuePresentKHR(
    const vkroots::VkQueueDispatch& pDispatch, 
    VkQueue queue, 
    const VkPresentInfoKHR* pPresentInfo)
{
    printf("Frame Finished. Presenting image: %u\n", 
           pPresentInfo->pImageIndices[0]);
    return pDispatch.QueuePresentKHR(queue, pPresentInfo);
}
```

This tells us when frames complete and what image was presented.

## Per-Frame Timeline

```
┌─────────────────────────── Frame N ───────────────────────────┐
│                                                                 │
├─ vkAcquireNextImageKHR                                          │
│  │ Returns: "Image 2 is available"                             │
│  │                                                              │
├─ [Game records rendering commands]                             │
│  │ vkBeginCommandBuffer                                        │
│  │ vkCmdBeginRendering / vkCmdBeginRenderPass                  │
│  │ vkCmdBindPipeline                                           │
│  │ vkCmdBindVertexBuffers                                      │
│  │ vkCmdDraw                                                   │
│  │ vkCmdEndRendering / vkCmdEndRenderPass                      │
│  │ vkEndCommandBuffer                                          │
│  │                                                              │
├─ vkQueueSubmit (Game's rendering)                              │
│  │ GPU executes game rendering commands                        │
│  │                                                              │
├─ [Overlay would render here - Future]                          │
│  │ vkQueueSubmit (Overlay rendering)                           │
│  │ GPU executes overlay commands                               │
│  │                                                              │
├─ vkQueuePresentKHR                                              │
│  │ GPU presents Image 2 to display                             │
│  │                                                              │
└─ Return to CPU for next frame ──────────────────────────────────┘
```

## Building Your Overlay

Once you have working hooks for all these phases, you can start creating Vulkan objects for the overlay:

1. **Shader Modules** - Compile vertex/fragment shaders
2. **Pipeline Layout** - Define shader parameters
3. **Graphics Pipeline** - Configure rasterization, blending, etc.
4. **Command Pool** - Create pool for command buffers
5. **Command Buffers** - Allocate buffers for recording commands
6. **Vertex Buffers** - Create geometry for overlay quads/text/etc.
7. **Descriptors** - Create descriptor sets for textures/samplers

## Recommended Reading

For in-depth understanding, refer to https://vulkan-tutorial.com/ with focus on:

- Instance
- Physical device
- Logical device and queues
- Swap chain
- Image views
- Graphics pipeline
- Command buffers
- Synchronization

Note: You can skip the window creation parts - the game/DXVK already creates the window and surface. Focus on the rendering objects and per-frame flow.

## Key Takeaways

1. The layer intercepts Vulkan function calls at strategic points
2. It captures handles and metadata needed for custom rendering
3. Device setup happens once (Phases 1-3)
4. Phases 4-6 repeat every frame
5. The overlay injection point is typically in Phase 5 (between submissions or within one)
6. Synchronization is critical to avoid GPU/CPU pipeline stalls
