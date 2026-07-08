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
#include <fstream>
#include <condition_variable>

// OpenSSL Cryptographic Components
#include <openssl/hmac.h>
#include <openssl/rc4.h>
#include <openssl/bn.h>
#include <openssl/sha.h>
#include <openssl/evp.h>

const size_t RING_BUFFER_SIZE = 512 * 1024;

// ============================================================================
// 1. CONDITION-VARIABLE SYNCHRONIZED SPSC RING BUFFER FRAMEWORK
// ============================================================================
class RingBuffer {
public:
    explicit RingBuffer(size_t size)
        : buffer(size + 1), capacity(size + 1), head(0), tail(0), closed(false) {}

    size_t write(const char* data, size_t len) {
        std::unique_lock<std::mutex> lock(mtx);
        size_t written = 0;
        while (written < len) {
            if (closed) break;
            size_t h = head; size_t t = tail;
            size_t available = (h > t) ? (h - t - 1) : (capacity - (t - h) - 1);
            if (available == 0) {
                cv_write.wait(lock, [this]() { return closed || (((head > tail) ? (head - tail - 1) : (capacity - (tail - head) - 1)) > 0); });
                continue;
            }
            size_t to_write = std::min(len - written, available);
            size_t to_end = capacity - t;
            size_t chunk = std::min(to_write, to_end);
            std::copy(data + written, data + written + chunk, buffer.begin() + t);
            tail = (t + chunk) % capacity;
            written += chunk;
            cv_read.notify_one();
        }
        return written;
    }

    size_t read_exactly(char* data, size_t len) {
        std::unique_lock<std::mutex> lock(mtx);
        size_t read_count = 0;
        while (read_count < len) {
            size_t h = head; size_t t = tail;
            size_t occupied = (t >= h) ? (t - h) : (capacity - (h - t));
            if (occupied == 0) {
                if (closed) break;
                cv_read.wait(lock, [this]() { return closed || (((tail >= head) ? (tail - head) : (capacity - (head - tail))) > 0); });
                continue;
            }
            size_t to_read = std::min(len - read_count, occupied);
            size_t to_end = capacity - h;
            size_t chunk = std::min(to_read, to_end);
            std::copy(buffer.begin() + h, buffer.begin() + h + chunk, data + read_count);
            head = (h + chunk) % capacity;
            read_count += chunk;
            cv_write.notify_one();
        }
        return read_count;
    }

    size_t read_some(char* data, size_t max_len) {
        std::unique_lock<std::mutex> lock(mtx);
        while (true) {
            size_t h = head; size_t t = tail;
            size_t occupied = (t >= h) ? (t - h) : (capacity - (h - t));
            if (occupied == 0) {
                if (closed) return 0;
                cv_read.wait(lock, [this]() { return closed || (((tail >= head) ? (tail - head) : (capacity - (head - tail))) > 0); });
                continue;
            }
            size_t to_read = std::min(max_len, occupied);
            size_t to_end = capacity - h;
            size_t chunk = std::min(to_read, to_end);
            std::copy(buffer.begin() + h, buffer.begin() + h + chunk, data);
            head = (h + chunk) % capacity;
            cv_write.notify_one();
            return chunk;
        }
    }

    void close() {
        std::lock_guard<std::mutex> lock(mtx);
        closed = true;
        cv_read.notify_all();
        cv_write.notify_all();
    }

private:
    std::vector<char> buffer;
    size_t capacity, head, tail;
    bool closed;
    std::mutex mtx;
    std::condition_variable cv_read, cv_write;
};

// ============================================================================
// 2. CORE TYPE DEFINITIONS & STRUCT CONTEXTS
// ============================================================================
enum class ProxyMode { TRANSPARENT, WOW_AUTH, FRAMED };
enum class StreamDirection { UPSTREAM, DOWNSTREAM };
enum class StreamParserState { EXPECTING_HEADER, EXPECTING_BODY };

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
    std::string auth_user;
    std::string auth_pass;
};

// ============================================================================
// 3. ACTIVE MITM SRP6 ENGINE (CORRECTED FOR WOW 3.3.5a)
// ============================================================================
class SRP6Mitm {
private:
    std::mutex mtx;
    std::string username, password;
    
    enum class MitmState { WAITING_CLIENT_CHALLENGE, WAITING_SERVER_CHALLENGE, WAITING_CLIENT_PROOF, WAITING_SERVER_PROOF, AUTH_COMPLETE };
    MitmState state = MitmState::WAITING_CLIENT_CHALLENGE;

    uint8_t salt[32];
    std::vector<uint8_t> N_bytes, g_bytes;
    BIGNUM *B_server = nullptr, *B_proxy = nullptr, *A_client = nullptr, *A_proxy = nullptr;
    BIGNUM *a_proxy = nullptr, *b_proxy = nullptr;
    
    uint8_t K_client[40], K_server[40];
    uint8_t M1_client[20], M2_client[20];

    static constexpr size_t SRP_KEY_SIZE = 32; 

    void sha1(const uint8_t* data, size_t len, uint8_t* out) { SHA1(data, len, out); }

    void bn_to_le(const BIGNUM* bn, uint8_t* out, size_t len) {
        std::memset(out, 0, len);
        int size = BN_num_bytes(bn);
        if (size <= 0) return;
        std::vector<uint8_t> temp(size);
        BN_bn2bin(bn, temp.data());
        for (int i = 0; i < size && i < (int)len; ++i) out[i] = temp[size - 1 - i];
    }

    BIGNUM* le_to_bn(const uint8_t* data, size_t len) {
        std::vector<uint8_t> temp(len);
        for (size_t i = 0; i < len; ++i) temp[i] = data[len - 1 - i];
        return BN_bin2bn(temp.data(), len, nullptr);
    }

    void compute_session_key(const BIGNUM* S, uint8_t* K) {
        uint8_t S_bytes[SRP_KEY_SIZE];
        std::memset(S_bytes, 0, sizeof(S_bytes));
        BN_bn2binpad(S, S_bytes, SRP_KEY_SIZE);
        uint8_t even[SRP_KEY_SIZE/2], odd[SRP_KEY_SIZE/2];
        for (size_t i = 0; i < SRP_KEY_SIZE/2; ++i) {
            even[i] = S_bytes[i * 2];
            odd[i] = S_bytes[i * 2 + 1];
        }
        uint8_t hash_even[20], hash_odd[20];
        sha1(even, SRP_KEY_SIZE/2, hash_even);
        sha1(odd, SRP_KEY_SIZE/2, hash_odd);
        std::memcpy(K, hash_even, 20);
        std::memcpy(K + 20, hash_odd, 20);
    }

    void compute_M1(const uint8_t* A, const uint8_t* B, const uint8_t* K, uint8_t* M1) {
        uint8_t hash_N[20], hash_g[20], hash_I[20];
        sha1(N_bytes.data(), N_bytes.size(), hash_N);
        sha1(g_bytes.data(), g_bytes.size(), hash_g);
        
        // CRITICAL: WoW uses UPPERCASE username for M1/M2 calculation
        std::string upper_user = username;
        std::transform(upper_user.begin(), upper_user.end(), upper_user.begin(), ::toupper);
        sha1((const uint8_t*)upper_user.c_str(), upper_user.length(), hash_I);

        uint8_t t1[20];
        for (int i = 0; i < 20; ++i) t1[i] = hash_N[i] ^ hash_g[i];

        SHA_CTX ctx; SHA1_Init(&ctx);
        SHA1_Update(&ctx, t1, 20);
        SHA1_Update(&ctx, hash_I, 20);
        SHA1_Update(&ctx, salt, 32);
        SHA1_Update(&ctx, A, SRP_KEY_SIZE);
        SHA1_Update(&ctx, B, SRP_KEY_SIZE);
        SHA1_Update(&ctx, K, 40);
        SHA1_Final(M1, &ctx);
    }

    void compute_M2(const uint8_t* A, const uint8_t* M1, const uint8_t* K, uint8_t* M2) {
        SHA_CTX ctx; SHA1_Init(&ctx);
        SHA1_Update(&ctx, A, SRP_KEY_SIZE);
        SHA1_Update(&ctx, M1, 20);
        SHA1_Update(&ctx, K, 40);
        SHA1_Final(M2, &ctx);
    }

    BIGNUM* calculate_S_as_client(const BIGNUM* A, const BIGNUM* B, const BIGNUM* private_key_a) {
        BN_CTX* ctx = BN_CTX_new();
        if (!ctx) return nullptr;
        BIGNUM* bn_N = BN_bin2bn(N_bytes.data(), N_bytes.size(), nullptr);
        BIGNUM* bn_g = BN_bin2bn(g_bytes.data(), g_bytes.size(), nullptr);
        BIGNUM* bn_k = BN_new(); BN_set_word(bn_k, 3);

        // CRITICAL: WoW uses LOWERCASE username for x calculation
        std::string user_pass = username + ":" + password;
        uint8_t hash_up[20]; sha1((const uint8_t*)user_pass.c_str(), user_pass.length(), hash_up);
        SHA_CTX x_ctx; SHA1_Init(&x_ctx); SHA1_Update(&x_ctx, salt, 32); SHA1_Update(&x_ctx, hash_up, 20);
        uint8_t x_hash[20]; SHA1_Final(x_hash, &x_ctx);
        BIGNUM* x = BN_bin2bn(x_hash, 20, nullptr);

        uint8_t A_net[SRP_KEY_SIZE], B_net[SRP_KEY_SIZE];
        bn_to_le(A, A_net, SRP_KEY_SIZE); bn_to_le(B, B_net, SRP_KEY_SIZE);
        SHA_CTX u_ctx; SHA1_Init(&u_ctx); SHA1_Update(&u_ctx, A_net, SRP_KEY_SIZE); SHA1_Update(&u_ctx, B_net, SRP_KEY_SIZE);
        uint8_t u_hash[20]; SHA1_Final(u_hash, &u_ctx);
        BIGNUM* u = BN_bin2bn(u_hash, 20, nullptr);

        BIGNUM* gx = BN_new(); BN_mod_exp(gx, bn_g, x, bn_N, ctx);
        BIGNUM* k_gx = BN_new(); BN_mod_mul(k_gx, bn_k, gx, bn_N, ctx);
        BIGNUM* B_minus_kgx = BN_new(); BN_mod_sub(B_minus_kgx, B, k_gx, bn_N, ctx);
        BIGNUM* ux = BN_new(); BN_mod_mul(ux, u, x, bn_N, ctx);
        BIGNUM* exponent = BN_new(); BN_add(exponent, private_key_a, ux);
        BIGNUM* S = BN_new(); BN_mod_exp(S, B_minus_kgx, exponent, bn_N, ctx);

        BN_clear_free(bn_N); BN_clear_free(bn_g); BN_clear_free(bn_k); BN_clear_free(x); BN_clear_free(u);
        BN_clear_free(gx); BN_clear_free(k_gx); BN_clear_free(B_minus_kgx); BN_clear_free(ux); BN_clear_free(exponent);
        BN_CTX_free(ctx);
        return S;
    }

    BIGNUM* calculate_S_as_server(const BIGNUM* A_client) {
        BN_CTX* ctx = BN_CTX_new();
        if (!ctx) return nullptr;
        BIGNUM* bn_N = BN_bin2bn(N_bytes.data(), N_bytes.size(), nullptr);
        BIGNUM* bn_g = BN_bin2bn(g_bytes.data(), g_bytes.size(), nullptr);

        std::string user_pass = username + ":" + password;
        uint8_t hash_up[20]; sha1((const uint8_t*)user_pass.c_str(), user_pass.length(), hash_up);
        SHA_CTX x_ctx; SHA1_Init(&x_ctx); SHA1_Update(&x_ctx, salt, 32); SHA1_Update(&x_ctx, hash_up, 20);
        uint8_t x_hash[20]; SHA1_Final(x_hash, &x_ctx);
        BIGNUM* x = BN_bin2bn(x_hash, 20, nullptr);

        BIGNUM* v = BN_new(); BN_mod_exp(v, bn_g, x, bn_N, ctx);

        uint8_t A_net[SRP_KEY_SIZE], B_net[SRP_KEY_SIZE];
        bn_to_le(A_client, A_net, SRP_KEY_SIZE); bn_to_le(B_proxy, B_net, SRP_KEY_SIZE);
        SHA_CTX u_ctx; SHA1_Init(&u_ctx); SHA1_Update(&u_ctx, A_net, SRP_KEY_SIZE); SHA1_Update(&u_ctx, B_net, SRP_KEY_SIZE);
        uint8_t u_hash[20]; SHA1_Final(u_hash, &u_ctx);
        BIGNUM* u = BN_bin2bn(u_hash, 20, nullptr);

        BIGNUM* vu = BN_new(); BN_mod_exp(vu, v, u, bn_N, ctx);
        BIGNUM* Avu = BN_new(); BN_mod_mul(Avu, A_client, vu, bn_N, ctx);
        BIGNUM* S = BN_new(); BN_mod_exp(S, Avu, b_proxy, bn_N, ctx);

        BN_clear_free(bn_N); BN_clear_free(bn_g); BN_clear_free(x); BN_clear_free(v); BN_clear_free(u);
        BN_clear_free(vu); BN_clear_free(Avu);
        BN_CTX_free(ctx);
        return S;
    }

public:
    SRP6Mitm(const std::string& user, const std::string& pass) : username(user), password(pass) {}
    ~SRP6Mitm() {
        if (B_server) BN_clear_free(B_server); if (B_proxy) BN_clear_free(B_proxy);
        if (A_client) BN_clear_free(A_client); if (A_proxy) BN_clear_free(A_proxy);
        if (a_proxy) BN_clear_free(a_proxy); if (b_proxy) BN_clear_free(b_proxy);
    }

    std::vector<uint8_t> process_upstream(const uint8_t* data, size_t len) {
        std::lock_guard<std::mutex> lock(mtx);
        if (state == MitmState::WAITING_CLIENT_CHALLENGE) {
            if (len > 34) {
                uint8_t I_len = data[33];
                if (I_len > 0 && I_len < 100 && (34 + I_len <= len)) {
                    username = std::string((char*)data + 34, I_len);
                    std::cout << "[MITM] Intercepted Username: '" << username << "'" << std::endl;
                }
            }
            state = MitmState::WAITING_SERVER_CHALLENGE;
            return {}; 
        }
        else if (state == MitmState::WAITING_CLIENT_PROOF) {
            A_client = le_to_bn(data + 1, SRP_KEY_SIZE);
            uint8_t client_M1[20]; std::memcpy(client_M1, data + 1 + SRP_KEY_SIZE, 20);

            BIGNUM* S_client = calculate_S_as_server(A_client);
            if (!S_client) return {};
            compute_session_key(S_client, K_client);
            BN_clear_free(S_client);

            uint8_t A_client_net[SRP_KEY_SIZE], B_proxy_net[SRP_KEY_SIZE];
            bn_to_le(A_client, A_client_net, SRP_KEY_SIZE); bn_to_le(B_proxy, B_proxy_net, SRP_KEY_SIZE);
            uint8_t expected_M1[20];
            compute_M1(A_client_net, B_proxy_net, K_client, expected_M1);

            if (std::memcmp(client_M1, expected_M1, 20) != 0) {
                std::cerr << "[MITM] Client M1 verification FAILED! Wrong password." << std::endl;
            }
            compute_M2(A_client_net, expected_M1, K_client, M2_client);

            a_proxy = BN_new(); BN_rand(a_proxy, 256, -1, 0);
            A_proxy = BN_new();
            BIGNUM* bn_N = BN_bin2bn(N_bytes.data(), N_bytes.size(), nullptr);
            BIGNUM* bn_g = BN_bin2bn(g_bytes.data(), g_bytes.size(), nullptr);
            BN_CTX* ctx = BN_CTX_new();
            BN_mod_exp(A_proxy, bn_g, a_proxy, bn_N, ctx);
            BN_clear_free(bn_N); BN_clear_free(bn_g); BN_CTX_free(ctx);

            BIGNUM* S_server = calculate_S_as_client(A_proxy, B_server, a_proxy);
            if (!S_server) return {};
            compute_session_key(S_server, K_server);
            BN_clear_free(S_server);

            uint8_t A_proxy_net[SRP_KEY_SIZE], B_server_net[SRP_KEY_SIZE];
            bn_to_le(A_proxy, A_proxy_net, SRP_KEY_SIZE); bn_to_le(B_server, B_server_net, SRP_KEY_SIZE);
            uint8_t M1_server[20];
            compute_M1(A_proxy_net, B_server_net, K_server, M1_server);

            state = MitmState::WAITING_SERVER_PROOF;

            std::vector<uint8_t> packet;
            packet.push_back(0x01);
            packet.insert(packet.end(), A_proxy_net, A_proxy_net + SRP_KEY_SIZE);
            packet.insert(packet.end(), M1_server, M1_server + 20);
            packet.insert(packet.end(), 20, 0); 
            packet.push_back(0x00); packet.push_back(0x00); 
            return packet;
        }
        return {};
    }

    std::vector<uint8_t> process_downstream(const uint8_t* data, size_t len) {
        std::lock_guard<std::mutex> lock(mtx);
        if (state == MitmState::WAITING_SERVER_CHALLENGE) {
            if (len < 40) return {};
            uint8_t error = data[2];
            if (error != 0) return {};

            size_t offset = 3;
            B_server = le_to_bn(data + offset, SRP_KEY_SIZE); offset += SRP_KEY_SIZE;
            uint8_t g_len = data[offset++]; g_bytes.assign(data + offset, data + offset + g_len); offset += g_len;
            uint8_t N_len = data[offset++]; N_bytes.assign(data + offset, data + offset + N_len); offset += N_len;
            std::memcpy(salt, data + offset, 32); offset += 32;

            b_proxy = BN_new(); BN_rand(b_proxy, 256, -1, 0);
            B_proxy = BN_new();
            BIGNUM* bn_N = BN_bin2bn(N_bytes.data(), N_bytes.size(), nullptr);
            BIGNUM* bn_g = BN_bin2bn(g_bytes.data(), g_bytes.size(), nullptr);
            BN_CTX* ctx = BN_CTX_new();
            BN_mod_exp(B_proxy, bn_g, b_proxy, bn_N, ctx);
            BN_clear_free(bn_N); BN_clear_free(bn_g); BN_CTX_free(ctx);

            state = MitmState::WAITING_CLIENT_PROOF;

            std::vector<uint8_t> packet;
            packet.push_back(0x00); packet.push_back(0x00); packet.push_back(0x00); 
            uint8_t B_proxy_net[SRP_KEY_SIZE];
            bn_to_le(B_proxy, B_proxy_net, SRP_KEY_SIZE);
            packet.insert(packet.end(), B_proxy_net, B_proxy_net + SRP_KEY_SIZE);
            packet.push_back(g_len); packet.insert(packet.end(), g_bytes.begin(), g_bytes.end());
            packet.push_back(N_len); packet.insert(packet.end(), N_bytes.begin(), N_bytes.end());
            packet.insert(packet.end(), salt, salt + 32);
            if (len > offset) packet.insert(packet.end(), data + offset, data + len);
            return packet;
        }
        else if (state == MitmState::WAITING_SERVER_PROOF) {
            if (len < 2) return {};
            uint8_t error = data[1];
            if (error == 0) {
                state = MitmState::AUTH_COMPLETE;
                std::cout << "[MITM] Auth Successful! Session Keys Derived." << std::endl;
                std::vector<uint8_t> packet;
                packet.push_back(0x01); packet.push_back(0x00); 
                packet.insert(packet.end(), M2_client, M2_client + 20);
                packet.insert(packet.end(), 4, 0); packet.insert(packet.end(), 2, 0); 
                return packet;
            }
            return {}; 
        }
        return {};
    }

    bool is_auth_complete() { std::lock_guard<std::mutex> lock(mtx); return state == MitmState::AUTH_COMPLETE; }
    void get_session_keys(uint8_t* out_client, uint8_t* out_server) {
        std::lock_guard<std::mutex> lock(mtx);
        std::memcpy(out_client, K_client, 40); std::memcpy(out_server, K_server, 40);
    }
};

// ============================================================================
// 4. PROTOCOL STREAM PARSING LAYER
// ============================================================================
size_t get_dynamic_auth_size(const char* header_accumulator, StreamDirection direction, bool& out_error) {
    out_error = false;
    uint8_t opcode = static_cast<uint8_t>(header_accumulator[0]);

    if (direction == StreamDirection::UPSTREAM) {
        if (opcode == 0x00) {
            uint16_t dynamic_len = static_cast<uint8_t>(header_accumulator[2]) | (static_cast<uint8_t>(header_accumulator[3]) << 8);
            return 4 + dynamic_len; 
        }
        if (opcode == 0x01) return 74; 
        if (opcode == 0x02) return 4;  
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

    // CORRECT WOTLK (3.3.5a) HMAC SEEDS EXTRACTED FROM GO IMPLEMENTATION
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
            size_t n = rb.read_exactly(packet_accumulator, 4);
            if (n < 4) break;

            bool protocol_fault = false;
            size_t total_packet_size = get_dynamic_auth_size(packet_accumulator, direction, protocol_fault);
            if (protocol_fault || total_packet_size > sizeof(packet_accumulator)) break;

            size_t remaining_bytes = total_packet_size - 4;
            if (remaining_bytes > 0) {
                if (rb.read_exactly(packet_accumulator + 4, remaining_bytes) < remaining_bytes) break;
            }

            std::vector<uint8_t> mitm_packet;
            if (direction == StreamDirection::UPSTREAM) mitm_packet = mitm_engine->process_upstream(reinterpret_cast<uint8_t*>(packet_accumulator), total_packet_size);
            else mitm_packet = mitm_engine->process_downstream(reinterpret_cast<uint8_t*>(packet_accumulator), total_packet_size);

            if (!mitm_packet.empty()) {
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
                    // DOWNSTREAM: Handle 5-byte "Large Header" edge case
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
                    // UPSTREAM: Little Endian size
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

// ============================================================================
// 5. NETWORK NETWORKING & CORE ORCHESTRATION ENGINE
// ============================================================================
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

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <mapping_config_string>\n";
        return 1;
    }
    std::vector<std::thread> listeners;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        std::stringstream ss(arg); std::string segment; std::vector<std::string> parts;
        while (std::getline(ss, segment, ':')) parts.push_back(segment);
        if (parts.size() < 3) continue;

        ProxyMapping mapping;
        mapping.listen_port = std::stoi(parts[0]);
        mapping.target_host = parts[1];
        mapping.target_port = parts[2];
        mapping.initial_mode = ProxyMode::TRANSPARENT;
        
        // CORRECTED WOTLK HEADER CONFIGS
        mapping.upstream = { 6, 4, false };  // Client -> Server: 6 bytes header, 4 bytes opcode, LE size
        mapping.downstream = { 4, 2, true }; // Server -> Client: 4 bytes header (can be 5), 2 bytes opcode, BE size

        if (parts.size() >= 4 && parts[3] == "a") mapping.initial_mode = ProxyMode::WOW_AUTH;
        if (parts.size() >= 6) { mapping.auth_user = parts[4]; mapping.auth_pass = parts[5]; }

        listeners.emplace_back(start_listener, mapping);
    }
    for (auto& t : listeners) if (t.joinable()) t.join();
    return 0;
}