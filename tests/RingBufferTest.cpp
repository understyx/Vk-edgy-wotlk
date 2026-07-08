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
    assert(rb.size() == 3);

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

void test_large_payload_no_deadlock() {
    // Buffer size is 100 bytes, payload is 1000 bytes.
    // read_exactly should consume incrementally and allow the producer to finish.
    RingBuffer rb(100);
    size_t payload_size = 1000;
    std::vector<char> send_data(payload_size, 'A');
    std::vector<char> recv_data(payload_size, 0);

    std::thread producer([&rb, &send_data]() {
        rb.write(send_data.data(), send_data.size());
    });

    std::thread consumer([&rb, &recv_data]() {
        rb.read_exactly(recv_data.data(), recv_data.size());
    });

    producer.join();
    consumer.join();

    assert(recv_data == send_data);
    std::cout << "test_large_payload_no_deadlock passed" << std::endl;
}

int main() {
    test_basic();
    test_read_exactly();
    test_size_and_peek();
    test_concurrency();
    test_large_payload_no_deadlock();
    return 0;
}
