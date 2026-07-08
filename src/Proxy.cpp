#include "Proxy.hpp"
#include <iostream>
#include <iomanip>
#include <thread>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>

size_t get_dynamic_auth_size(const char* header_accumulator, StreamDirection direction, bool& out_error) {
    out_error = false;
    uint8_t opcode = static_cast<uint8_t>(header_accumulator[0]);

    if (direction == StreamDirection::UPSTREAM) {
        if (opcode == 0x00) {
            uint16_t dynamic_len = static_cast<uint8_t>(header_accumulator[2]) | (static_cast<uint8_t>(header_accumulator[3]) << 8);
            return 4 + dynamic_len;
        }
        if (opcode == 0x01) return 75; // Updated size
        if (opcode == 0x02) { // Read dynamic size at offset 2
            uint16_t dynamic_len = static_cast<uint8_t>(header_accumulator[2]) | (static_cast<uint8_t>(header_accumulator[3]) << 8);
            return 4 + dynamic_len;
        }
        if (opcode == 0x03) return 57;
    }
    else {
        if (opcode == 0x00) {
            uint8_t error = header_accumulator[2];
            if (error == 0) {
                size_t base_total = 119;
                uint8_t security_flags = static_cast<uint8_t>(header_accumulator[118]);
                size_t extra = 0;
                if (security_flags & 0x01) extra += 20;
                if (security_flags & 0x02) extra += 12;
                if (security_flags & 0x04) extra += 1;
                return base_total + extra;
            }
            return 3;
        }
        if (opcode == 0x01) {
            uint8_t error = header_accumulator[1];
            if (error == 0) return 32;
            return 4;
        }
        if (opcode == 0x02) return 34;
        if (opcode == 0x03) return 2;
    }
    out_error = true;
    return 0;
}

void initialize_session_crypto(SessionContext& context, const uint8_t* raw_session_key) {
    std::lock_guard<std::mutex> lock(context.mtx);
    if (context.crypto_active) return;

    const uint8_t upstream_seed[] = {0xC2, 0xB3, 0x72, 0x3C, 0xC6, 0xAE, 0xD9, 0xB5, 0x34, 0x3C, 0x53, 0xEE, 0x2F, 0x43, 0x67, 0xCE};
    const uint8_t downstream_seed[] = {0xCC, 0x98, 0xAE, 0x04, 0xE8, 0x97, 0xEA, 0xCA, 0x12, 0xDD, 0xC0, 0x93, 0x42, 0x91, 0x53, 0x57};

    uint8_t key_out[20];
    unsigned int key_len = 0;

    HMAC(EVP_sha1(), upstream_seed, sizeof(upstream_seed), raw_session_key, 40, key_out, &key_len);
    RC4_set_key(&context.upstream_rc4, 20, key_out);

    HMAC(EVP_sha1(), downstream_seed, sizeof(downstream_seed), raw_session_key, 40, key_out, &key_len);
    RC4_set_key(&context.downstream_rc4, 20, key_out);

    uint8_t flush_buffer[1024];
    std::memset(flush_buffer, 0, sizeof(flush_buffer));
    RC4(&context.upstream_rc4, sizeof(flush_buffer), flush_buffer, flush_buffer);
    RC4(&context.downstream_rc4, sizeof(flush_buffer), flush_buffer, flush_buffer);

    context.crypto_active = true;
}

void writer_thread(int to_fd, RingBuffer& rb, std::shared_ptr<SessionContext> context, std::shared_ptr<SRP6Mitm> mitm_engine, DirectionConfig config, StreamDirection direction) {
    char packet_accumulator[65536];
    std::string dir_str = (direction == StreamDirection::UPSTREAM) ? "[C->S]" : "[S->C]";
    StreamParserState parser_state = StreamParserState::EXPECTING_HEADER;
    size_t expected_body_length = 0;
    size_t current_header_size = config.header_size;

    while (true) {
        ProxyMode current_mode;
        bool is_encrypted = false;
        {
            std::lock_guard<std::mutex> lock(context->mtx);
            current_mode = context->mode;
            is_encrypted = context->crypto_active;
        }

        if (current_mode == ProxyMode::TRANSPARENT) {
            size_t n = rb.read_some(packet_accumulator, sizeof(packet_accumulator));
            if (n == 0) break;
            if (send(to_fd, packet_accumulator, n, 0) <= 0) break;
        }
        else if (current_mode == ProxyMode::WOW_AUTH) {
            // Read first 4 bytes to check Opcode and layout signatures
            size_t n = rb.read_exactly(packet_accumulator, 4);
            if (n < 4) {
                std::cerr << dir_str << " [WOW_AUTH] Failed to read initial 4-byte header token." << std::endl;
                break;
            }

            bool protocol_fault = false;
            size_t total_packet_size = get_dynamic_auth_size(packet_accumulator, direction, protocol_fault);
            
            std::cout << dir_str << " [WOW_AUTH_DIAG] Intercepted Opcode: 0x" 
                      << std::hex << (int)(static_cast<uint8_t>(packet_accumulator[0])) 
                      << " | Calculated Total Size: " << std::dec << total_packet_size 
                      << " | Fault Flag: " << protocol_fault << std::endl;

            if (protocol_fault) {
                std::cerr << dir_str << " [FATAL] get_dynamic_auth_size reported a protocol alignment fault for opcode 0x" 
                          << std::hex << (int)(static_cast<uint8_t>(packet_accumulator[0])) << std::dec << "!" << std::endl;
                // Dump hex headers to inspect the exact format structural drift
                std::cerr << "  -> Raw Header Hex dump: ";
                for(size_t i=0; i<4; ++i) std::cerr << std::hex << std::setw(2) << std::setfill('0') << (int)(static_cast<uint8_t>(packet_accumulator[i])) << " ";
                std::cerr << std::dec << std::endl;
                break;
            }

            if (total_packet_size > sizeof(packet_accumulator)) {
                std::cerr << dir_str << " [FATAL] Calculated packet size (" << total_packet_size << ") exceeds proxy frame buffers!" << std::endl;
                break;
            }

            size_t remaining_bytes = total_packet_size - 4;
            if (remaining_bytes > 0) {
                size_t read_bytes = rb.read_exactly(packet_accumulator + 4, remaining_bytes);
                if (read_bytes < remaining_bytes) {
                    std::cerr << dir_str << " [WOW_AUTH] Stream broke while reading remaining packet payload body." << std::endl;
                    break;
                }
            }

            std::vector<uint8_t> mitm_packet;
            if (direction == StreamDirection::UPSTREAM) {
                mitm_packet = mitm_engine->process_upstream(reinterpret_cast<uint8_t*>(packet_accumulator), total_packet_size);
            } else {
                mitm_packet = mitm_engine->process_downstream(reinterpret_cast<uint8_t*>(packet_accumulator), total_packet_size);
            }

            if (!mitm_packet.empty()) {
                std::cout << dir_str << " [WOW_AUTH] Injecting modified MITM verification frame (" << mitm_packet.size() << " bytes)." << std::endl;
                if (send(to_fd, mitm_packet.data(), mitm_packet.size(), 0) <= 0) break;
            } else {
                if (send(to_fd, packet_accumulator, total_packet_size, 0) <= 0) break;
            }

            if (mitm_engine->is_auth_complete()) {
                uint8_t tmp_k_client[40], tmp_k_server[40];
                mitm_engine->get_session_keys(tmp_k_client, tmp_k_server);
                std::lock_guard<std::mutex> lock(context->mtx);
                context->mode = ProxyMode::FRAMED;
                initialize_session_crypto(*context, tmp_k_client);
                std::cout << "[MITM_SUCCESS] Transitioning proxy session engine cleanly to ProxyMode::FRAMED" << std::endl;
            }
        }
        else if (current_mode == ProxyMode::FRAMED) {
            if (parser_state == StreamParserState::EXPECTING_HEADER) {
                current_header_size = config.header_size;
                size_t n = rb.read_exactly(packet_accumulator, current_header_size);
                if (n < current_header_size) break;

                uint8_t decrypted_header[8];
                std::memcpy(decrypted_header, packet_accumulator, current_header_size);

                if (is_encrypted) {
                    std::lock_guard<std::mutex> lock(context->mtx);
                    RC4_KEY* key = (direction == StreamDirection::UPSTREAM) ? &context->upstream_rc4 : &context->downstream_rc4;
                    RC4(key, current_header_size, decrypted_header, decrypted_header);
                }

                size_t composite_size = 0;
                size_t opcode_offset = 2;

                if (config.is_big_endian_len) {
                    if (decrypted_header[0] & 0x80) {
                        char extra_byte;
                        if (rb.read_exactly(&extra_byte, 1) < 1) break;
                        packet_accumulator[4] = extra_byte;
                        if (is_encrypted) {
                            std::lock_guard<std::mutex> lock(context->mtx);
                            RC4(&context->downstream_rc4, 1, reinterpret_cast<uint8_t*>(&extra_byte), reinterpret_cast<uint8_t*>(&extra_byte));
                            packet_accumulator[4] = extra_byte;
                            decrypted_header[4] = extra_byte;
                        }
                        current_header_size = 5;
                        opcode_offset = 3;
                        composite_size = ((static_cast<unsigned char>(decrypted_header[0]) & 0x7F) << 16) |
                                         (static_cast<unsigned char>(decrypted_header[1]) << 8) |
                                         static_cast<unsigned char>(decrypted_header[2]);
                    } else {
                        composite_size = (static_cast<unsigned char>(decrypted_header[0]) << 8) | static_cast<unsigned char>(decrypted_header[1]);
                    }
                } else {
                    composite_size = static_cast<unsigned char>(decrypted_header[0]) | (static_cast<unsigned char>(decrypted_header[1]) << 8);
                }

                if (composite_size >= config.opcode_size) {
                    expected_body_length = composite_size - config.opcode_size;
                } else {
                    std::cerr << dir_str << " Protocol Framing Error." << std::endl;
                    break;
                }

                uint32_t opcode = 0;
                if (config.opcode_size == 4) std::memcpy(&opcode, decrypted_header + opcode_offset, 4);
                else if (config.opcode_size == 2) {
                    uint16_t op16; std::memcpy(&op16, decrypted_header + opcode_offset, 2); opcode = op16;
                }

                std::cout << dir_str << " Operational Plane Opcode: 0x" << std::hex << std::setw(config.opcode_size * 2) << std::setfill('0') << opcode
                          << std::dec << " | Length: " << expected_body_length << " bytes" << std::endl;

                if (expected_body_length > sizeof(packet_accumulator) - current_header_size) break;
                parser_state = StreamParserState::EXPECTING_BODY;
            }

            if (parser_state == StreamParserState::EXPECTING_BODY) {
                if (expected_body_length > 0) {
                    size_t n = rb.read_exactly(packet_accumulator + current_header_size, expected_body_length);
                    if (n < expected_body_length) break;
                }
                size_t total_frame_size = current_header_size + expected_body_length;
                if (send(to_fd, packet_accumulator, total_frame_size, 0) <= 0) break;
                parser_state = StreamParserState::EXPECTING_HEADER;
            }
        }
    }
}

void bridge(int from_fd, int to_fd, RingBuffer& rb, std::shared_ptr<SessionContext> context, std::shared_ptr<SRP6Mitm> mitm_engine, DirectionConfig config, StreamDirection direction) {
    std::thread reader([from_fd, &rb]() {
        char buf[8192];
        while (true) {
            ssize_t n = recv(from_fd, buf, sizeof(buf), 0);
            if (n <= 0) break;
            if (rb.write(buf, n) < (size_t)n) break;
        }
        rb.close();
    });

    writer_thread(to_fd, rb, context, mitm_engine, config, direction);
    rb.close();
    ::shutdown(from_fd, SHUT_RD);
    if (reader.joinable()) reader.join();
    ::shutdown(to_fd, SHUT_WR);
}

void handle_client(int client_fd, ProxyMapping mapping) {
    struct addrinfo hints, *res;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(mapping.target_host.c_str(), mapping.target_port.c_str(), &hints, &res) != 0) { close(client_fd); return; }
    int target_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (target_fd < 0) { freeaddrinfo(res); close(client_fd); return; }
    if (connect(target_fd, res->ai_addr, res->ai_addrlen) < 0) { freeaddrinfo(res); close(target_fd); close(client_fd); return; }
    freeaddrinfo(res);

    auto client_to_target = std::make_unique<RingBuffer>(RING_BUFFER_SIZE);
    auto target_to_client = std::make_unique<RingBuffer>(RING_BUFFER_SIZE);
    auto context = std::make_shared<SessionContext>(mapping.initial_mode);
    auto mitm_engine = std::make_shared<SRP6Mitm>(mapping.auth_user, mapping.auth_pass);

    std::thread t1(bridge, client_fd, target_fd, std::ref(*client_to_target), context, mitm_engine, mapping.upstream, StreamDirection::UPSTREAM);
    std::thread t2(bridge, target_fd, client_fd, std::ref(*target_to_client), context, mitm_engine, mapping.downstream, StreamDirection::DOWNSTREAM);

    t1.join(); t2.join();
    close(target_fd); close(client_fd);
}

void start_listener(ProxyMapping mapping) {
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) return;
    int opt = 1; setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in serv_addr; std::memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET; serv_addr.sin_addr.s_addr = INADDR_ANY; serv_addr.sin_port = htons(mapping.listen_port);
    if (bind(listen_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) { close(listen_fd); return; }
    listen(listen_fd, 10);
    while (true) {
        int client_fd = accept(listen_fd, nullptr, nullptr);
        if (client_fd < 0) continue;
        std::thread(handle_client, client_fd, mapping).detach();
    }
    close(listen_fd);
}