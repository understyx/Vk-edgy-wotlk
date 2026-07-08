CXX = g++
CXXFLAGS = -std=c++17 -Wall -pthread

all: mitmproxy test

mitmproxy: main.cpp
	g++ -std=c++17 -Wall -Wno-deprecated-declarations -pthread main.cpp -o mitmproxy -lcrypto

test: RingBufferTest.cpp RingBuffer.hpp
	$(CXX) $(CXXFLAGS) RingBufferTest.cpp -o test_rb
	./test_rb

clean:
	rm -f mitmproxy test_rb
