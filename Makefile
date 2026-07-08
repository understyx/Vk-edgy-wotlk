CXX = g++
CXXFLAGS = -std=c++11 -Wall -pthread

all: mitmproxy test

mitmproxy: main.cpp RingBuffer.hpp
	$(CXX) $(CXXFLAGS) main.cpp -o mitmproxy

test: RingBufferTest.cpp RingBuffer.hpp
	$(CXX) $(CXXFLAGS) RingBufferTest.cpp -o test_rb
	./test_rb

clean:
	rm -f mitmproxy test_rb
