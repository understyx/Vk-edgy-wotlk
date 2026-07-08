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

void test_wrap_around() {
    size_t size = 10;
    RingBuffer rb(size);

    // Write 7 bytes
    rb.write("1234567", 7);
    // Read 5 bytes. Head at 5, Tail at 7. Count 2.
    char buf[10];
    rb.read(buf, 5);

    // Write 6 bytes. This should wrap around.
    // 3 bytes fit at end (7,8,9), 3 bytes at beginning (0,1,2).
    rb.write("ABCDEF", 6);

    size_t n = rb.read(buf, 8);
    buf[n] = '\0';
    assert(n == 8);
    assert(strcmp(buf, "67ABCDEF") == 0);
    std::cout << "test_wrap_around passed" << std::endl;
}

void test_blocking() {
    RingBuffer rb(5);
    std::thread t([&rb]() {
        rb.write("12345", 5);
        // This should block until someone reads
        rb.write("6", 1);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    char buf[1];
    size_t n = rb.read(buf, 1);
    assert(n == 1);
    assert(buf[0] == '1');

    t.join();
    n = rb.read(buf, 1);
    assert(n == 1);
    assert(buf[0] == '2');
    std::cout << "test_blocking passed" << std::endl;
}

int main() {
    test_basic();
    test_wrap_around();
    test_blocking();
    return 0;
}
