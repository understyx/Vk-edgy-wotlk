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
    size_t n = rb.read_exactly(buf, strlen(msg));
    buf[n] = '\0';

    assert(n == strlen(msg));
    assert(strcmp(msg, buf) == 0);
    std::cout << "test_basic passed" << std::endl;
}

void test_read_exactly() {
    RingBuffer rb(1024);
    std::thread t([&rb]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        rb.write("header", 6);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        rb.write("body", 4);
    });

    char buf[10];
    size_t n = rb.read_exactly(buf, 10);
    assert(n == 10);
    assert(strncmp(buf, "headerbody", 10) == 0);
    t.join();
    std::cout << "test_read_exactly passed" << std::endl;
}

void test_size_and_peek() {
    RingBuffer rb(10);
    rb.write("abc", 3);
    assert(rb.size() == 3);

    char buf[3];
    size_t p = rb.peek(buf, 2);
    assert(p == 2);
    assert(buf[0] == 'a' && buf[1] == 'b');
    assert(rb.size() == 3); // size should not change after peek

    std::cout << "test_size_and_peek passed" << std::endl;
}

void test_concurrency() {
    const int count = 100000;
    RingBuffer rb(1024);
    std::thread producer([&rb, count]() {
        for (int i = 0; i < count; ++i) {
            rb.write((char*)&i, sizeof(int));
        }
    });

    std::thread consumer([&rb, count]() {
        for (int i = 0; i < count; ++i) {
            int val;
            rb.read_exactly((char*)&val, sizeof(int));
            assert(val == i);
        }
    });

    producer.join();
    consumer.join();
    std::cout << "test_concurrency passed" << std::endl;
}

int main() {
    test_basic();
    test_read_exactly();
    test_size_and_peek();
    test_concurrency();
    return 0;
}
