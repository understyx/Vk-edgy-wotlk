#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "RingBuffer.hpp"

const size_t RING_BUFFER_SIZE = 512 * 1024;

void bridge(int from_fd, int to_fd, RingBuffer& rb) {
    std::thread reader([from_fd, &rb]() {
        char buf[8192];
        while (true) {
            ssize_t n = recv(from_fd, buf, sizeof(buf), 0);
            if (n <= 0) break;
            if (rb.write(buf, n) < (size_t)n) break;
        }
        rb.close();
        shutdown(from_fd, SHUT_RD);
    });

    std::thread writer([to_fd, &rb]() {
        char buf[8192];
        while (true) {
            size_t n = rb.read(buf, sizeof(buf));
            if (n == 0) break;
            ssize_t sent = send(to_fd, buf, n, 0);
            if (sent <= 0) break;
        }
        shutdown(to_fd, SHUT_WR);
    });

    reader.join();
    writer.join();
}

void handle_client(int client_fd, std::string target_host, std::string target_port) {
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(target_host.c_str(), target_port.c_str(), &hints, &res) != 0) {
        std::cerr << "Error: could not resolve " << target_host << ":" << target_port << std::endl;
        close(client_fd);
        return;
    }

    int target_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (target_fd < 0) {
        perror("Error opening target socket");
        freeaddrinfo(res);
        close(client_fd);
        return;
    }

    if (connect(target_fd, res->ai_addr, res->ai_addrlen) < 0) {
        perror("Error connecting to target");
        freeaddrinfo(res);
        close(target_fd);
        close(client_fd);
        return;
    }

    freeaddrinfo(res);

    RingBuffer client_to_target(RING_BUFFER_SIZE);
    RingBuffer target_to_client(RING_BUFFER_SIZE);

    std::thread t1(bridge, client_fd, target_fd, std::ref(client_to_target));
    std::thread t2(bridge, target_fd, client_fd, std::ref(target_to_client));

    t1.join();
    t2.join();

    close(target_fd);
    close(client_fd);
    std::cout << "Connection closed" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " <listen_port> <target_host> <target_port>" << std::endl;
        return 1;
    }

    int listen_port = std::stoi(argv[1]);
    std::string target_host = argv[2];
    std::string target_port = argv[3];

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("Error opening listen socket");
        return 1;
    }

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(listen_port);

    if (bind(listen_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Error on binding");
        return 1;
    }

    listen(listen_fd, 5);
    std::cout << "Listening on port " << listen_port << ", proxying to " << target_host << ":" << target_port << std::endl;

    while (true) {
        struct sockaddr_in cli_addr;
        socklen_t clilen = sizeof(cli_addr);
        int client_fd = accept(listen_fd, (struct sockaddr*)&cli_addr, &clilen);
        if (client_fd < 0) {
            perror("Error on accept");
            continue;
        }

        std::cout << "Accepted connection" << std::endl;
        std::thread(handle_client, client_fd, target_host, target_port).detach();
    }

    close(listen_fd);
    return 0;
}
