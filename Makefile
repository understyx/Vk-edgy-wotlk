CXX = g++
CXXFLAGS = -std=c++20 -Ithird_party/vulkan-headers/include -Iincludes -Wall -Wextra -fPIC -L.

# Source files
LAYER_SRCS = src/main.cpp src/game_data/GameDataQuery.cpp src/ultralight_renderer/WebUIRenderingEngine.cpp src/vulkan_renderer/VulkanOverlayRenderer.cpp
DEMO_SRCS = src/demo_pipeline.cpp src/game_data/GameDataQuery.cpp src/ultralight_renderer/WebUIRenderingEngine.cpp src/vulkan_renderer/VulkanOverlayRenderer.cpp

# Targets
LAYER_TARGET = libvklayer_understyx_wotlk.so
DEMO_TARGET = demo_pipeline

.PHONY: all clean

all: $(LAYER_TARGET) $(DEMO_TARGET)

$(LAYER_TARGET): $(LAYER_SRCS)
	$(CXX) $(CXXFLAGS) -shared $(LAYER_SRCS) -o $(LAYER_TARGET) -lvulkan

$(DEMO_TARGET): $(DEMO_SRCS)
	$(CXX) $(CXXFLAGS) $(DEMO_SRCS) -o $(DEMO_TARGET) -lvulkan

demo_pipeline: $(DEMO_TARGET)

clean:
	rm -f $(LAYER_TARGET) $(DEMO_TARGET) libvulkan.so
