CXX = g++
CXXFLAGS = -std=c++17 -Wall -pthread

all: mitmproxy test demo_pipeline

mitmproxy: main.cpp
	g++ -std=c++17 -Wall -Wno-deprecated-declarations -pthread main.cpp -o mitmproxy -lcrypto

test: RingBufferTest.cpp RingBuffer.hpp
	$(CXX) $(CXXFLAGS) RingBufferTest.cpp -o test_rb
	./test_rb

demo_pipeline: demo_pipeline.cpp common/GameDataTypes.hpp common/PointerSwapBuffer.hpp game_data/GameData.hpp ultralight_renderer/UltralightStubs.hpp ultralight_renderer/UltralightRenderer.hpp vulkan_renderer/VulkanRenderer.hpp
	$(CXX) $(CXXFLAGS) demo_pipeline.cpp -o demo_pipeline

clean:
	rm -f mitmproxy test_rb demo_pipeline
