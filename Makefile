CXX = g++
CXXFLAGS = -std=c++17 -Wall -pthread

all: mitmproxy test_rb_bin libVkModernUILayer.so test_vulkan_layer_bin

mitmproxy: main.cpp
	g++ -std=c++17 -Wall -Wno-deprecated-declarations -pthread main.cpp -o mitmproxy -lcrypto

# Compile the Vulkan layer as a shared library
libVkModernUILayer.so: VkModernUILayer.cpp VkModernUILayer.hpp
	$(CXX) $(CXXFLAGS) -fPIC -shared VkModernUILayer.cpp -o libVkModernUILayer.so

# Compile the Vulkan layer test runner
test_vulkan_layer_bin: test_vulkan_layer.cpp libVkModernUILayer.so
	$(CXX) $(CXXFLAGS) test_vulkan_layer.cpp -o test_vulkan_layer -L. -lVkModernUILayer -Wl,-rpath,.

test_rb_bin: RingBufferTest.cpp RingBuffer.hpp
	$(CXX) $(CXXFLAGS) RingBufferTest.cpp -o test_rb

test: test_rb_bin test_vulkan_layer_bin
	./test_rb
	./test_vulkan_layer

clean:
	rm -f mitmproxy test_rb libVkModernUILayer.so test_vulkan_layer
