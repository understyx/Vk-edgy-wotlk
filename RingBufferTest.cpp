#include "RingBuffer.hpp"
#include <iostream>
#include <thread>
#include <cassert>
#include <cstring>
#include <vector>

void test_basic() {
    RingBuffer rb(1024);
    const char* msg = "hello world";
    rb.write(msg, strlen(msg));

    char buf[64];
    size_t n = rb.read(buf, strlen(msg));
    buf[n] = '\0';

    assert(n == strlen(msg));
    assert(strcmp(msg, buf) == 0);
    std::cout << "test_basic passed" << std::endl;
}

void test_read_exactly() {
    RingBuffer rb(1024);
    std::thread t([&rb]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        rb.write("header", 6);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        rb.write("body", 4);
    });

    char buf[10];
    size_t n = rb.read_exactly(buf, 10);
    assert(n == 10);
    assert(strncmp(buf, "headerbody", 10) == 0);
    t.join();
    std::cout << "test_read_exactly passed" << std::endl;
}

void test_size() {
    RingBuffer rb(1024);
    assert(rb.size() == 0);
    rb.write("data", 4);
    assert(rb.size() == 4);
    char buf[2];
    rb.read(buf, 2);
    assert(rb.size() == 2);
    std::cout << "test_size passed" << std::endl;
}

int main() {
    test_basic();
    test_read_exactly();
    test_size();
    return 0;
}
