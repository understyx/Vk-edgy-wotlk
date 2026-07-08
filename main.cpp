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
#include <mutex>
#include <memory>
#include <algorithm>
#include <chrono>
#include <new>
#include <atomic>

// OpenSSL Cryptographic Components
#include <openssl/hmac.h>
#include <openssl/rc4.h>


// Global Configurations
const size_t RING_BUFFER_SIZE = 512 * 1024; // <--- ADD THIS FOR THE SCOPE ERROR
// ============================================================================
// 1. LOCK-FREE CACHE-ALIGNED SPSC RING BUFFER FRAMEWORK
// ============================================================================
class RingBuffer {
public:
    explicit RingBuffer(size_t size)
        : buffer(size + 1), capacity(size + 1), head(0), tail(0), closed(false) {}

    // Writer path (Single Producer)
    size_t write(const char* data, size_t len) {
        size_t written = 0;
        size_t local_t = tail.load(std::memory_order_relaxed);
        size_t spin_count = 0;

        while (written < len) {
            if (closed.load(std::memory_order_acquire)) break;

            size_t h = head.load(std::memory_order_acquire);
            size_t available = (h > local_t) ? (h - local_t - 1) : (capacity - (local_t - h) - 1);

            if (available == 0) {
                adaptive_backoff(spin_count);
                continue;
            }

            spin_count = 0;
            size_t to_write = std::min(len - written, available);
            size_t to_end = capacity - local_t;
            size_t chunk = std::min(to_write, to_end);

            std::copy(data + written, data + written + chunk, buffer.begin() + local_t);

            local_t = (local_t + chunk) % capacity;
            tail.store(local_t, std::memory_order_release);
            written += chunk;
        }
        return written;
    }

    // Reader path (Single Consumer) - Precise frame allocation block
    size_t read_exactly(char* data, size_t len) {
        size_t read_count = 0;
        size_t spin_count = 0;

        while (read_count < len) {
            size_t h = head.load(std::memory_order_relaxed);
            size_t t = tail.load(std::memory_order_acquire);

            size_t occupied = (t >= h) ? (t - h) : (capacity - (h - t));

            if (occupied == 0) {
                if (closed.load(std::memory_order_acquire)) break;
                adaptive_backoff(spin_count);
                continue;
            }

            spin_count = 0;
            size_t to_read = std::min(len - read_count, occupied);
            size_t to_end = capacity - h;
            size_t chunk = std::min(to_read, to_end);

            std::copy(buffer.begin() + h, buffer.begin() + h + chunk, data + read_count);

            head.store((h + chunk) % capacity, std::memory_order_release);
            read_count += chunk;
        }
        return read_count;
    }

    // Reader path (Single Consumer) - High-throughput slice collection
    size_t read_some(char* data, size_t max_len) {
        size_t spin_count = 0;
        while (true) {
            size_t h = head.load(std::memory_order_relaxed);
            size_t t = tail.load(std::memory_order_acquire);

            size_t occupied = (t >= h) ? (t - h) : (capacity - (h - t));

            if (occupied == 0) {
                if (closed.load(std::memory_order_acquire)) return 0;
                adaptive_backoff(spin_count);
                continue;
            }

            size_t to_read = std::min(max_len, occupied);
            size_t to_end = capacity - h;
            size_t chunk = std::min(to_read, to_end);

            std::copy(buffer.begin() + h, buffer.begin() + h + chunk, data);

            head.store((h + chunk) % capacity, std::memory_order_release);
            return chunk;
        }
    }

    void close() {
        closed.store(true, std::memory_order_release);
    }

private:
    void adaptive_backoff(size_t& spin_count) const {
        if (spin_count < 10) {
            spin_count++;
            std::this_thread::yield();
        } else if (spin_count < 100) {
            spin_count++;
            std::this_thread::sleep_for(std::chrono::microseconds(50));
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

#ifdef __cpp_lib_hardware_interference_size
    static constexpr size_t CacheLineSize = std::hardware_destructive_interference_size;
#else
    static constexpr size_t CacheLineSize = 64;
#endif

    std::vector<char> buffer;
    size_t capacity;

    alignas(CacheLineSize) std::atomic<size_t> head;
    alignas(CacheLineSize) std::atomic<size_t> tail;
    alignas(CacheLineSize) std::atomic<bool> closed;
};

// ============================================================================
// 2. CORE TYPE DEFINITIONS & STRUC CONTEXTS
// ============================================================================
enum class ProxyMode {
    TRANSPARENT,
    WOW_AUTH,
    FRAMED
};

enum class StreamDirection {
    UPSTREAM,
    DOWNSTREAM
};

enum class StreamParserState {
    EXPECTING_HEADER,
    EXPECTING_BODY
};

struct DirectionConfig {
    size_t header_size;
    size_t opcode_size;
    bool is_big_endian_len;
};

struct SessionContext {
    std::mutex mtx;
    ProxyMode mode;
    bool crypto_active;
    RC4_KEY upstream_rc4;
    RC4_KEY downstream_rc4;

    SessionContext(ProxyMode initial_mode) 
        : mode(initial_mode), crypto_active(false) {
        std::memset(&upstream_rc4, 0, sizeof(RC4_KEY));
        std::memset(&downstream_rc4, 0, sizeof(RC4_KEY));
    }
};

struct ProxyMapping {
    int listen_port;
    std::string target_host;
    std::string target_port;
    ProxyMode initial_mode;
    DirectionConfig upstream;
    DirectionConfig downstream;
};

// ============================================================================
// 3. PROTOCOL STREAM PARSING LAYER
// ============================================================================

// Evaluates the initial 4-byte segment of Logon streams to identify variable bounds
size_t get_dynamic_auth_size(char* header_accumulator, StreamDirection direction, bool& out_error) {
    out_error = false;
    uint8_t opcode = static_cast<uint8_t>(header_accumulator[0]);

    if (direction == StreamDirection::UPSTREAM) {
        // Variable client frames containing text-string components
        if (opcode == 0x00 || opcode == 0x02) {
            uint16_t dynamic_len = 0;
            std::memcpy(&dynamic_len, header_accumulator + 2, 2);
            return 4 + dynamic_len; 
        }
        if (opcode == 0x01) return 73; // Static Proof sizing
        if (opcode == 0x03) return 57;
    } 
    else { 
        uint8_t result_code = static_cast<uint8_t>(header_accumulator[1]);
        if (opcode == 0x00) { 
            if (result_code == 0) return 44; // Active Challenge Success
            return 3;                        // Diagnostic Structural Failure Code
        }
        if (opcode == 0x01) { 
            if (result_code == 0) return 34; // Active Proof Verification Success
            return 3;
        }
        if (opcode == 0x02) return 20;
        if (opcode == 0x03) return 5;
    }

    out_error = true;
    return 0;
}

void initialize_session_crypto(SessionContext& context, const uint8_t* raw_session_key) {
    std::lock_guard<std::mutex> lock(context.mtx);
    if (context.crypto_active) return;

    const uint8_t upstream_seed[] = {0x38, 0xA7, 0x83, 0x15, 0x94, 0x1F, 0x53, 0xFA, 0x47, 0x3D, 0x2C, 0x5E, 0x48, 0x84, 0x6E, 0x8A};
    const uint8_t downstream_seed[] = {0xC2, 0xB3, 0x72, 0x3C, 0xC6, 0xAE, 0xD9, 0xB5, 0x34, 0x3C, 0x53, 0xEE, 0x2F, 0x43, 0x67, 0xCE};

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
    std::cout << "[SYSTEM] Cryptographic framework processing initialized." << std::endl;
}

void writer_thread(int to_fd, RingBuffer& rb, std::shared_ptr<SessionContext> context, DirectionConfig config, StreamDirection direction) {
    char packet_accumulator[65536];
    std::string dir_str = (direction == StreamDirection::UPSTREAM) ? "[C->S]" : "[S->C]";
    StreamParserState parser_state = StreamParserState::EXPECTING_HEADER;
    size_t expected_body_length = 0;

    while (true) {
        ProxyMode current_mode;
        bool is_encrypted = false;
        {
            std::lock_guard<std::mutex> lock(context->mtx);
            current_mode = context->mode;
            is_encrypted = context->crypto_active;
        }


        // --- PATH A: DYNAMIC TRANSPARENT -> FRAMED UPGRADE ROUTE ---
        if (current_mode == ProxyMode::TRANSPARENT) {
            size_t n = rb.read_some(packet_accumulator, sizeof(packet_accumulator));
            if (n == 0) break;

            // Only run parser checks if we aren't handling a standard logon link (Port 3724)
            if (config.header_size > 0) { 
                size_t cursor = 0;
                while (cursor + 3 < n) {
                    // World opcodes are 2 bytes Downstream, 4 bytes Upstream.
                    if (direction == StreamDirection::DOWNSTREAM) {
                        // SMSG_AUTH_CHALLENGE is usually the first packet sent by the world server
                        uint16_t opcode = (static_cast<unsigned char>(packet_accumulator[cursor]) << 8) | 
                                        static_cast<unsigned char>(packet_accumulator[cursor + 1]);
                        
                        if (opcode == 0x01EC) { // SMSG_AUTH_CHALLENGE
                            std::cout << dir_str << " Handshake Intercept: SMSG_AUTH_CHALLENGE (0x01EC)" << std::endl;
                        }
                        else if (opcode == 0x01EE) { // SMSG_AUTH_RESPONSE
                            std::cout << dir_str << " Handshake Intercept: SMSG_AUTH_RESPONSE (0x01EE) -> Session Auth Success!" << std::endl;
                            std::cout << "[SYSTEM] Activating Stateful Asymmetric Framing Engine." << std::endl;
                            
                            // Trigger mode upgrade to start parsing standard game loops
                            std::lock_guard<std::mutex> lock(context->mtx);
                            context->mode = ProxyMode::FRAMED;
                            // If you want to enable encryption logging here, uncomment below:
                            // context->crypto_active = true; 
                            break;
                        }
                    }
                    else { // StreamDirection::UPSTREAM
                        uint32_t opcode;
                        std::memcpy(&opcode, packet_accumulator + cursor, 4);
                        if (opcode == 0x01ED) { // CMSG_AUTH_SESSION
                            std::cout << dir_str << " Handshake Intercept: CMSG_AUTH_SESSION (0x01ED)" << std::endl;
                        }
                    }
                    cursor++;
                }
            }

            if (send(to_fd, packet_accumulator, n, 0) <= 0) break;
        }
        // --- PATH B: HYBRID DYNAMIC AUTH SNOOP & REALM REDIRECTION ROUTE ---
        else if (current_mode == ProxyMode::WOW_AUTH) {
            size_t n = rb.read_some(packet_accumulator, sizeof(packet_accumulator));
            if (n == 0) break;

            // Check if this is a Downstream Server-to-Client packet stream
            if (direction == StreamDirection::DOWNSTREAM) {
                std::string target_needle = "145.239.161.30:8093";
                auto it = std::search(packet_accumulator, packet_accumulator + n, target_needle.begin(), target_needle.end());
                
                if (it != packet_accumulator + n) {
                    size_t offset = std::distance(packet_accumulator, it);
                    std::string local_redirect = "127.0.0.1:8085";

                    std::cout << "[S->C] Intercepted Realm List payload. Mutating target vector..." << std::endl;

                    // 1. Calculate the contraction difference
                    size_t old_len = target_needle.length();
                    size_t new_len = local_redirect.length();
                    size_t delta = old_len - new_len; // 19 - 14 = 5 bytes

                    // 2. Overwrite the IP string memory block
                    std::memcpy(packet_accumulator + offset, local_redirect.c_str(), new_len);

                    // 3. Shift all remaining bytes leftward to compact the gap
                    size_t remaining_bytes_after = n - (offset + old_len);
                    if (remaining_bytes_after > 0) {
                        std::memmove(packet_accumulator + offset + new_len, 
                                    packet_accumulator + offset + old_len, 
                                    remaining_bytes_after);
                    }

                    // 4. Shrink the stream allocation tracker by the delta
                    n -= delta;

                    // 5. Update the WoW 16-bit packet size header field at offset 1-2
                    if (n >= 3 && static_cast<uint8_t>(packet_accumulator[0]) == 0x10) {
                        uint16_t protocol_len = 0;
                        std::memcpy(&protocol_len, packet_accumulator + 1, 2);
                        protocol_len -= delta;
                        std::memcpy(packet_accumulator + 1, &protocol_len, 2);
                    }

                    std::cout << "[SUCCESS] Packet compacted safely. Client redirected locally to :8085." << std::endl;
                }
            }

            // Standard telemetry opcode visual snoop loop
            size_t cursor = 0;
            while (cursor < n) {
                uint8_t potential_opcode = static_cast<uint8_t>(packet_accumulator[cursor]);
                if (direction == StreamDirection::UPSTREAM && (potential_opcode == 0x00 || potential_opcode == 0x01)) {
                    std::cout << dir_str << " Detected Auth Opcode: 0x" << std::hex << (int)potential_opcode << std::dec << std::endl;
                    break;
                }
                else if (direction == StreamDirection::DOWNSTREAM && (potential_opcode == 0x00 || potential_opcode == 0x01)) {
                    int result_code = (cursor + 1 < n) ? static_cast<uint8_t>(packet_accumulator[cursor + 1]) : -1;
                    std::cout << dir_str << " Detected Server Response: 0x" << std::hex << (int)potential_opcode << " (Status: " << std::dec << result_code << ")" << std::endl;
                    break;
                }
                cursor++;
            }

            if (send(to_fd, packet_accumulator, n, 0) <= 0) break;
        }
        // --- PATH C: STATEFUL BOUNDED ASYMMETRIC FRAMING ROUTE ---
        else if (current_mode == ProxyMode::FRAMED) {
            if (parser_state == StreamParserState::EXPECTING_HEADER) {
                size_t n = rb.read_exactly(packet_accumulator, config.header_size);
                if (n < config.header_size) break;

                if (is_encrypted) {
                    std::lock_guard<std::mutex> lock(context->mtx);
                    RC4_KEY* key = (direction == StreamDirection::UPSTREAM) ? &context->upstream_rc4 : &context->downstream_rc4;
                    RC4(key, config.header_size, reinterpret_cast<uint8_t*>(packet_accumulator), reinterpret_cast<uint8_t*>(packet_accumulator));
                }

                size_t composite_size = 0;
                if (config.is_big_endian_len) {
                    composite_size = (static_cast<unsigned char>(packet_accumulator[0]) << 8) | static_cast<unsigned char>(packet_accumulator[1]);
                } else {
                    composite_size = static_cast<unsigned char>(packet_accumulator[0]) | (static_cast<unsigned char>(packet_accumulator[1]) << 8);
                }

                if (composite_size >= config.opcode_size) {
                    expected_body_length = composite_size - config.opcode_size;
                } else {
                    std::cerr << dir_str << " Protocol Error: Structural framing underflow." << std::endl;
                    break;
                }

                uint32_t opcode = 0;
                if (config.opcode_size == 4) {
                    std::memcpy(&opcode, packet_accumulator + 2, 4);
                } else if (config.opcode_size == 2) {
                    uint16_t op16;
                    std::memcpy(&op16, packet_accumulator + 2, 2);
                    opcode = op16;
                }

                std::cout << dir_str << " Data Plane Opcode: 0x" << std::hex << std::setw(config.opcode_size * 2) << std::setfill('0') << opcode
                          << std::dec << " | Length: " << expected_body_length << " bytes" << std::endl;

                if (expected_body_length > sizeof(packet_accumulator) - config.header_size) {
                    std::cerr << dir_str << " Boundary Error: Segment length violates MTU limits." << std::endl;
                    break;
                }

                parser_state = StreamParserState::EXPECTING_BODY;
            }

            if (parser_state == StreamParserState::EXPECTING_BODY) {
                if (expected_body_length > 0) {
                    size_t n = rb.read_exactly(packet_accumulator + config.header_size, expected_body_length);
                    if (n < expected_body_length) break;
                }

                if (is_encrypted) {
                    std::lock_guard<std::mutex> lock(context->mtx);
                    RC4_KEY* key = (direction == StreamDirection::UPSTREAM) ? &context->upstream_rc4 : &context->downstream_rc4;
                    RC4(key, config.header_size, reinterpret_cast<uint8_t*>(packet_accumulator), reinterpret_cast<uint8_t*>(packet_accumulator));
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

// ============================================================================
// 4. NETWORK NETWORKING & CORE ORCHESTRATION ENGINE
// ============================================================================
void bridge(int from_fd, int to_fd, RingBuffer& rb, std::shared_ptr<SessionContext> context, DirectionConfig config, StreamDirection direction) {
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

    writer_thread(to_fd, rb, context, config, direction);
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

    auto context = std::make_shared<SessionContext>(mapping.initial_mode);

    std::thread t1(bridge, client_fd, target_fd, std::ref(client_to_target), context, mapping.upstream, StreamDirection::UPSTREAM);
    std::thread t2(bridge, target_fd, client_fd, std::ref(target_to_client), context, mapping.downstream, StreamDirection::DOWNSTREAM);

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
        perror("[ERROR] Bind constraints failed");
        return;
    }

    listen(listen_fd, 10);
    std::string mode_str = "TRANSPARENT";
    if (mapping.initial_mode == ProxyMode::FRAMED) mode_str = "FRAMED";
    if (mapping.initial_mode == ProxyMode::WOW_AUTH) mode_str = "WOW_AUTH";

    std::cout << "[SYSTEM] Listener spawned on :" << mapping.listen_port << " routing to " 
              << mapping.target_host << ":" << mapping.target_port << " [" << mode_str << "]" << std::endl;

    while (true) {
        int client_fd = accept(listen_fd, NULL, NULL);
        if (client_fd < 0) continue;
        std::thread(handle_client, client_fd, mapping).detach();
    }
}

int main() {
    std::vector<std::thread> listeners;

    // Listener 1: The Logon Authentication Pipeline
    ProxyMapping logon_map;
    logon_map.listen_port = 3724;
    logon_map.target_host = "logon.warmane.com";
    logon_map.target_port = "3724";
    logon_map.initial_mode = ProxyMode::WOW_AUTH;
    listeners.emplace_back(start_listener, logon_map);

    // Listener 2: The Bounded Game World Plane (Local Redirection Target)
    ProxyMapping world_map;
    world_map.listen_port = 8085;
    world_map.target_host = "145.239.161.30";
    world_map.target_port = "8093";
    world_map.initial_mode = ProxyMode::TRANSPARENT; // <--- Start transparently to allow the raw handshake to pass

    // Asymmetric runtime sizes for post-auth tracking
    world_map.upstream = {6, 4, true};   
    world_map.downstream = {4, 2, true};
    listeners.emplace_back(start_listener, world_map);

    for (auto& t : listeners) {
        t.join();
    }
    return 0;
}