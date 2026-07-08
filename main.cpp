#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sstream>
#include <iomanip>
#include "RingBuffer.hpp"

const size_t RING_BUFFER_SIZE = 512 * 1024;

enum class ProxyMode {
    TRANSPARENT,
    FRAMED
};

enum class StreamDirection {
    UPSTREAM,   // Client -> Server
    DOWNSTREAM  // Server -> Client
};

struct DirectionConfig {
    size_t header_size;
    size_t opcode_size;
    bool is_big_endian_len;
};

struct ProxyMapping {
    int listen_port;
    std::string target_host;
    std::string target_port;
    ProxyMode mode;
    DirectionConfig upstream;
    DirectionConfig downstream;
};

enum class StreamParserState {
    EXPECTING_HEADER,
    EXPECTING_BODY
};

void writer_thread(int to_fd, RingBuffer& rb, ProxyMode mode, DirectionConfig config, StreamDirection direction) {
    char packet_accumulator[65536];
    std::string dir_str = (direction == StreamDirection::UPSTREAM) ? "[C->S]" : "[S->C]";

    if (mode == ProxyMode::TRANSPARENT) {
        while (true) {
            size_t n = rb.read_some(packet_accumulator, sizeof(packet_accumulator));
            if (n == 0) break;
            if (send(to_fd, packet_accumulator, n, 0) <= 0) break;
        }
    } else {
        StreamParserState parser_state = StreamParserState::EXPECTING_HEADER;
        size_t expected_body_length = 0;

        while (true) {
            if (parser_state == StreamParserState::EXPECTING_HEADER) {
                size_t n = rb.read_exactly(packet_accumulator, config.header_size);
                if (n < config.header_size) break;

                // Length parsing (Big Endian)
                size_t composite_size = (static_cast<unsigned char>(packet_accumulator[0]) << 8) |
                                         static_cast<unsigned char>(packet_accumulator[1]);

                // Adjustment: composite size includes opcode field
                if (composite_size >= config.opcode_size) {
                    expected_body_length = composite_size - config.opcode_size;
                } else {
                    std::cerr << dir_str << " Protocol error: composite size smaller than opcode size" << std::endl;
                    break;
                }

                // Opcode extraction (Little Endian)
                uint32_t opcode = 0;
                if (config.opcode_size == 4) {
                    std::memcpy(&opcode, packet_accumulator + 2, 4);
                } else if (config.opcode_size == 2) {
                    uint16_t op16;
                    std::memcpy(&op16, packet_accumulator + 2, 2);
                    opcode = op16;
                }

                std::cout << dir_str << " Opcode: 0x" << std::hex << std::setw(config.opcode_size * 2) << std::setfill('0') << opcode
                          << std::dec << " Payload: " << expected_body_length << " bytes" << std::endl;

                if (expected_body_length > sizeof(packet_accumulator) - config.header_size) {
                    std::cerr << dir_str << " Protocol error: payload too large (" << expected_body_length << ")" << std::endl;
                    break;
                }

                parser_state = StreamParserState::EXPECTING_BODY;
            }

            if (parser_state == StreamParserState::EXPECTING_BODY) {
                if (expected_body_length > 0) {
                    size_t n = rb.read_exactly(packet_accumulator + config.header_size, expected_body_length);
                    if (n < expected_body_length) break;
                }

                size_t total_frame_size = config.header_size + expected_body_length;
                if (send(to_fd, packet_accumulator, total_frame_size, 0) <= 0) break;

                parser_state = StreamParserState::EXPECTING_HEADER;
            }
        }
    }
    rb.close();
    shutdown(to_fd, SHUT_WR);
}

void bridge(int from_fd, int to_fd, RingBuffer& rb, ProxyMode mode, DirectionConfig config, StreamDirection direction) {
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

    writer_thread(to_fd, rb, mode, config, direction);
    reader.join();
}

void handle_client(int client_fd, ProxyMapping mapping) {
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(mapping.target_host.c_str(), mapping.target_port.c_str(), &hints, &res) != 0) {
        close(client_fd);
        return;
    }

    int target_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (target_fd < 0) {
        freeaddrinfo(res);
        close(client_fd);
        return;
    }

    if (connect(target_fd, res->ai_addr, res->ai_addrlen) < 0) {
        freeaddrinfo(res);
        close(target_fd);
        close(client_fd);
        return;
    }
    freeaddrinfo(res);

    RingBuffer client_to_target(RING_BUFFER_SIZE);
    RingBuffer target_to_client(RING_BUFFER_SIZE);

    std::thread t1(bridge, client_fd, target_fd, std::ref(client_to_target), mapping.mode, mapping.upstream, StreamDirection::UPSTREAM);
    std::thread t2(bridge, target_fd, client_fd, std::ref(target_to_client), mapping.mode, mapping.downstream, StreamDirection::DOWNSTREAM);

    t1.join();
    t2.join();

    close(target_fd);
    close(client_fd);
}

void start_listener(ProxyMapping mapping) {
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(mapping.listen_port);

    if (bind(listen_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Error on binding");
        return;
    }

    listen(listen_fd, 5);
    std::cout << "Proxying port " << mapping.listen_port << " to " << mapping.target_host << ":" << mapping.target_port
              << " [" << (mapping.mode == ProxyMode::TRANSPARENT ? "TRANSPARENT" : "FRAMED") << "]" << std::endl;

    while (true) {
        int client_fd = accept(listen_fd, NULL, NULL);
        if (client_fd < 0) continue;
        std::thread(handle_client, client_fd, mapping).detach();
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <mapping1> [<mapping2> ...]" << std::endl;
        std::cerr << "Mapping format: listen_port:target_host:target_port[:mode]" << std::endl;
        std::cerr << "Mode: 't' for transparent (default), 'f' for framed" << std::endl;
        return 1;
    }

    std::vector<std::thread> listeners;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        std::stringstream ss(arg);
        std::string segment;
        std::vector<std::string> parts;
        while (std::getline(ss, segment, ':')) {
            parts.push_back(segment);
        }

        if (parts.size() < 3) {
            std::cerr << "Invalid mapping: " << arg << std::endl;
            continue;
        }

        ProxyMapping mapping;
        mapping.listen_port = std::stoi(parts[0]);
        mapping.target_host = parts[1];
        mapping.target_port = parts[2];
        mapping.mode = ProxyMode::TRANSPARENT;

        if (parts.size() >= 4 && parts[3] == "f") {
            mapping.mode = ProxyMode::FRAMED;
        }

        // Asymmetric framing configurations
        mapping.upstream = {6, 4, true};   // Client -> Server: 2-byte BE len, 4-byte LE opcode
        mapping.downstream = {4, 2, true}; // Server -> Client: 2-byte BE len, 2-byte LE opcode

        listeners.emplace_back(start_listener, mapping);
    }

    for (auto& t : listeners) {
        t.join();
    }

    return 0;
}
