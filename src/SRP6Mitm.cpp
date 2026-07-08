#include "SRP6Mitm.hpp"
#include <iostream>
#include <iomanip>
#include <cstring>
#include <algorithm>
#include <sstream>
#include <openssl/sha.h>
#include <openssl/bn.h>

static std::string to_hex(const uint8_t* data, size_t len) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (size_t i = 0; i < len; ++i) {
        ss << std::setw(2) << static_cast<int>(data[i]);
    }
    return ss.str();
}

static std::string bn_to_hex(const BIGNUM* bn) {
    if (!bn) return "NULL";
    char* hex = BN_bn2hex(bn);
    std::string str(hex);
    OPENSSL_free(hex);
    return str;
}

SRP6Mitm::SRP6Mitm(const std::string& user, const std::string& pass) 
    : username(user), password(pass) 
{
    std::memset(salt, 0, sizeof(salt));
    std::memset(K_client, 0, sizeof(K_client));
    std::memset(K_server, 0, sizeof(K_server));
    std::memset(M2_client, 0, sizeof(M2_client)); // Added for state safety
}

SRP6Mitm::~SRP6Mitm() {
    if (B_server) BN_clear_free(B_server);
    if (B_proxy) BN_clear_free(B_proxy);
    if (A_client) BN_clear_free(A_client);
    if (A_proxy) BN_clear_free(A_proxy);
    if (a_proxy) BN_clear_free(a_proxy);
    if (b_proxy) BN_clear_free(b_proxy);
}

void SRP6Mitm::sha1(const uint8_t* data, size_t len, uint8_t* out) {
    SHA_CTX ctx;
    SHA1_Init(&ctx);
    SHA1_Update(&ctx, data, len);
    SHA1_Final(out, &ctx);
}

void SRP6Mitm::bn_to_le(const BIGNUM* bn, uint8_t* out, size_t len)
{
    memset(out,0,len);
    int bytes = BN_num_bytes(bn);
    if(bytes > (int)len)
    {
        std::cerr << "BN overflow: "
                  << bytes << " bytes\n";
        bytes=len;
    }
    std::vector<uint8_t> tmp(bytes);
    BN_bn2bin(bn,tmp.data());
    for(int i=0;i<bytes;i++)
    {
        out[i]=tmp[bytes-1-i];
    }
}

BIGNUM* SRP6Mitm::le_to_bn(const uint8_t* data, size_t len) {
    std::vector<uint8_t> be(len);
    for (size_t i = 0; i < len; ++i) {
        be[i] = data[len - 1 - i]; // Correct: Reverses LE wire to BE for OpenSSL
    }
    return BN_bin2bn(be.data(), len, nullptr);
}

void SRP6Mitm::compute_session_key(const BIGNUM* S, uint8_t* K)
{
    uint8_t S_be[SRP_KEY_SIZE];
    memset(S_be,0,sizeof(S_be));

    int bn_bytes = BN_num_bytes(S);

    if (bn_bytes > SRP_KEY_SIZE)
    {
        std::cerr << "[ERROR] SRP S too large: "
                  << bn_bytes << " bytes\n";

        // keep only least significant 32 bytes
        uint8_t tmp[64];
        memset(tmp,0,sizeof(tmp));

        BN_bn2bin(S,tmp);

        memcpy(S_be,
            tmp + (bn_bytes - SRP_KEY_SIZE),
            SRP_KEY_SIZE
        );
    }
    else
    {
        BN_bn2bin(
            S,
            S_be + (SRP_KEY_SIZE - bn_bytes)
        );
    }


    uint8_t even[SRP_KEY_SIZE / 2], odd[SRP_KEY_SIZE / 2];
    for (size_t i = 0; i < SRP_KEY_SIZE / 2; ++i) {
        even[i] = S_be[i * 2];
        odd[i]  = S_be[i * 2 + 1];
    }

    uint8_t hash_even[20], hash_odd[20];
    sha1(even, SRP_KEY_SIZE / 2, hash_even);
    sha1(odd, SRP_KEY_SIZE / 2, hash_odd);

    for (size_t i = 0; i < 20; ++i) {
        K[i * 2]     = hash_even[i];
        K[i * 2 + 1] = hash_odd[i];
    }
}

void SRP6Mitm::compute_M1(const uint8_t* A_raw, const uint8_t* B_raw, const uint8_t* K, uint8_t* M1) {
    uint8_t hash_N[20], hash_g[20], hash_I[20];
    
    // H(N): Big-Endian Padded Prime
    BIGNUM* bn_N = le_to_bn(N_bytes.data(), N_bytes.size());
    uint8_t N_be[SRP_KEY_SIZE];
    BN_bn2binpad(bn_N, N_be, SRP_KEY_SIZE);
    sha1(N_be, SRP_KEY_SIZE, hash_N);
    BN_clear_free(bn_N);

    // H(g): 32-byte Big-Endian Padded
    uint8_t g_padded[SRP_KEY_SIZE] = {0};
    g_padded[SRP_KEY_SIZE - 1] = g_bytes[0]; 
    sha1(g_padded, SRP_KEY_SIZE, hash_g);

    // H(I): Username only (Uppercase)
    std::string upper_user = username;
    std::transform(upper_user.begin(), upper_user.end(), upper_user.begin(), ::toupper);
    sha1((const uint8_t*)upper_user.c_str(), upper_user.length(), hash_I);

    uint8_t t1[20];
    for (int i = 0; i < 20; ++i) t1[i] = hash_N[i] ^ hash_g[i];

    // DECISIVE DIAGNOSTIC: Print these and compare to a known trace
    std::cout << "[DEBUG][M1_INPUTS]\n"
              << "  -> H(N) XOR H(g): " << to_hex(t1, 20) << "\n"
              << "  -> H(Identity):   " << to_hex(hash_I, 20) << "\n"
              << "  -> K:             " << to_hex(K, 40) << std::endl;

    SHA_CTX ctx; 
    SHA1_Init(&ctx);
    SHA1_Update(&ctx, t1, 20);
    SHA1_Update(&ctx, hash_I, 20);
    SHA1_Update(&ctx, salt, 32);
    SHA1_Update(&ctx, A_raw, SRP_KEY_SIZE);
    SHA1_Update(&ctx, B_raw, SRP_KEY_SIZE);
    SHA1_Update(&ctx, K, 40);
    SHA1_Final(M1, &ctx);
}
void SRP6Mitm::compute_M2(const uint8_t* A, const uint8_t* M1, const uint8_t* K, uint8_t* M2) {
    SHA_CTX ctx;
    SHA1_Init(&ctx);
    SHA1_Update(&ctx, A, SRP_KEY_SIZE);
    SHA1_Update(&ctx, M1, 20);
    SHA1_Update(&ctx, K, 40);
    SHA1_Final(M2, &ctx);
}

BIGNUM* SRP6Mitm::calculate_S_as_server(const BIGNUM* A_client) {
    BN_CTX* ctx = BN_CTX_new();
    BIGNUM* bn_N = le_to_bn(N_bytes.data(), N_bytes.size());
    BIGNUM* bn_g = le_to_bn(g_bytes.data(), g_bytes.size());

    std::string upper_user = username; std::transform(upper_user.begin(), upper_user.end(), upper_user.begin(), ::toupper);
    std::string upper_pass = password; std::transform(upper_pass.begin(), upper_pass.end(), upper_pass.begin(), ::toupper);
    std::string user_pass = upper_user + ":" + upper_pass;
    
    uint8_t hash_up[20]; sha1((const uint8_t*)user_pass.c_str(), user_pass.length(), hash_up);
    
    SHA_CTX x_ctx; SHA1_Init(&x_ctx);
    SHA1_Update(&x_ctx, salt, 32); 
    SHA1_Update(&x_ctx, hash_up, 20);
    uint8_t x_hash[20]; SHA1_Final(x_hash, &x_ctx);
    uint8_t x_le[20];
for (int i = 0; i < 20; i++) x_le[i] = x_hash[19 - i]; // Reverse for LE
BIGNUM* x = BN_bin2bn(x_le, 20, nullptr);

    BIGNUM* v = BN_new(); BN_mod_exp(v, bn_g, x, bn_N, ctx);

    // FIX 2: Compute u over the immutable wire bytes stored from packets
    SHA_CTX u_ctx; SHA1_Init(&u_ctx);
    SHA1_Update(&u_ctx, A_client_raw.data(), SRP_KEY_SIZE);
    SHA1_Update(&u_ctx, B_proxy_raw.data(), SRP_KEY_SIZE);
    uint8_t u_hash[20]; SHA1_Final(u_hash, &u_ctx);
    BIGNUM* u = BN_bin2bn(u_hash, 20, nullptr);

    BIGNUM* vu = BN_new(); BN_mod_exp(vu, v, u, bn_N, ctx);
    BIGNUM* Avu = BN_new(); BN_mod_mul(Avu, A_client, vu, bn_N, ctx);
    BIGNUM* S = BN_new(); BN_mod_exp(S, Avu, b_proxy, bn_N, ctx);

    BN_clear_free(bn_N); BN_clear_free(bn_g); BN_clear_free(x); BN_clear_free(v); BN_clear_free(u);
    BN_clear_free(vu); BN_clear_free(Avu); BN_CTX_free(ctx);
    return S;
}

BIGNUM* SRP6Mitm::calculate_S_as_client(const BIGNUM* A, const BIGNUM* B, const BIGNUM* private_key_a) {
    BN_CTX* ctx = BN_CTX_new();
    BIGNUM* bn_N = le_to_bn(N_bytes.data(), N_bytes.size()); // Protocol array: LE
    BIGNUM* bn_g = le_to_bn(g_bytes.data(), g_bytes.size()); // Protocol array: LE
    BIGNUM* bn_k = BN_new(); BN_set_word(bn_k, 3);

    std::string upper_user = username; std::transform(upper_user.begin(), upper_user.end(), upper_user.begin(), ::toupper);
    std::string upper_pass = password; std::transform(upper_pass.begin(), upper_pass.end(), upper_pass.begin(), ::toupper);
    std::string user_pass = upper_user + ":" + upper_pass;
    
    uint8_t hash_up[20]; sha1((const uint8_t*)user_pass.c_str(), user_pass.length(), hash_up);
    SHA_CTX x_ctx; SHA1_Init(&x_ctx); SHA1_Update(&x_ctx, salt, 32); SHA1_Update(&x_ctx, hash_up, 20);
    uint8_t x_hash[20]; SHA1_Final(x_hash, &x_ctx);
    
    // FIX: SHA-1 digest must be interpreted as a Big-Endian integer
    uint8_t x_le[20];
for (int i = 0; i < 20; i++) x_le[i] = x_hash[19 - i]; // Reverse for LE
BIGNUM* x = BN_bin2bn(x_le, 20, nullptr);

    uint8_t A_le[SRP_KEY_SIZE], B_le[SRP_KEY_SIZE];
    bn_to_le(A, A_le, SRP_KEY_SIZE);
    bn_to_le(B, B_le, SRP_KEY_SIZE);

    SHA_CTX u_ctx; SHA1_Init(&u_ctx);
    SHA1_Update(&u_ctx, A_le, SRP_KEY_SIZE);
    SHA1_Update(&u_ctx, B_le, SRP_KEY_SIZE);
    uint8_t u_hash[20]; SHA1_Final(u_hash, &u_ctx);
    
    // FIX: SHA-1 digest must be interpreted as a Big-Endian integer
    BIGNUM* u = BN_bin2bn(u_hash, 20, nullptr);

    BIGNUM* gx = BN_new(); BN_mod_exp(gx, bn_g, x, bn_N, ctx);
    BIGNUM* k_gx = BN_new(); BN_mod_mul(k_gx, bn_k, gx, bn_N, ctx);
    
    BIGNUM* B_minus_kgx = BN_new(); 
    BN_mod_sub(B_minus_kgx, B, k_gx, bn_N, ctx);
    if (BN_is_negative(B_minus_kgx)) {
        BN_add(B_minus_kgx, B_minus_kgx, bn_N);
    }

    BIGNUM* ux = BN_new(); BN_mod_mul(ux, u, x, bn_N, ctx);
    BIGNUM* exponent = BN_new(); BN_add(exponent, private_key_a, ux);
    BIGNUM* S = BN_new(); BN_mod_exp(S, B_minus_kgx, exponent, bn_N, ctx);

    BN_clear_free(bn_N); BN_clear_free(bn_g); BN_clear_free(bn_k); BN_clear_free(x); BN_clear_free(u);
    BN_clear_free(gx); BN_clear_free(k_gx); BN_clear_free(B_minus_kgx); BN_clear_free(ux); BN_clear_free(exponent);
    BN_CTX_free(ctx);
    return S;
}

std::vector<uint8_t> SRP6Mitm::process_upstream(const uint8_t* data, size_t len) {
    std::lock_guard<std::mutex> lock(mtx);
    if (len == 0) return {};

    uint8_t opcode = data[0];
    std::cout << "[TRACE][UPSTREAM] Client -> Server | Opcode: 0x" 
              << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(opcode) 
              << std::dec << " | Length: " << len << " | Current State: " << static_cast<int>(state) << std::endl;

    if (state == MitmState::WAITING_CLIENT_CHALLENGE) {
        if (opcode == 0x00 && len > 34) {
            uint8_t I_len = data[33];
            if (I_len > 0 && I_len < 100 && (static_cast<size_t>(34) + I_len <= len)) {
                username = std::string((char*)data + 34, I_len);
                std::cout << "[MITM] Intercepted Username: '" << username << "'" << std::endl;
            }
        }
        state = MitmState::WAITING_SERVER_CHALLENGE;
        return std::vector<uint8_t>(data, data + len);
    }
    else if (state == MitmState::WAITING_CLIENT_PROOF) {
        if (opcode != 0x01) return std::vector<uint8_t>(data, data + len);
        
        A_client_raw.assign(data + 1, data + 1 + SRP_KEY_SIZE);
        A_client = le_to_bn(A_client_raw.data(), SRP_KEY_SIZE);
        uint8_t client_M1[20]; std::memcpy(client_M1, data + 1 + SRP_KEY_SIZE, 20);

        BIGNUM* S_client = calculate_S_as_server(A_client);
        if (!S_client) return {};
        compute_session_key(S_client, K_client);
        std::cout << "[DEBUG] K-Verification:\n"
          << "  -> S_client (BN): " << bn_to_hex(S_client) << "\n"
          << "  -> K_client (Hex): " << to_hex(K_client, 40) << std::endl;
        BN_clear_free(S_client);

        uint8_t A_client_net[SRP_KEY_SIZE], B_proxy_net[SRP_KEY_SIZE];
        bn_to_le(A_client, A_client_net, SRP_KEY_SIZE); bn_to_le(B_proxy, B_proxy_net, SRP_KEY_SIZE);
        uint8_t expected_M1[20];
        //compute_M1(A_client_net, B_proxy_net, K_client, expected_M1);

        compute_M1(A_client_raw.data(), B_proxy_raw.data(), K_client, expected_M1);
        uint8_t B_proxy_wire[SRP_KEY_SIZE];
        bn_to_le(B_proxy, B_proxy_wire, SRP_KEY_SIZE);

        std::cout << "[DEBUG] Transcript Check:\n"
                << "  -> A raw:        " << to_hex(A_client_raw.data(), 32) << "\n"
                << "  -> B proxy raw:  " << to_hex(B_proxy_raw.data(), 32) << "\n"
                << "  -> B proxy wire: " << to_hex(B_proxy_wire, 32) << std::endl;

        std::cout << "[DEBUG] Verification Array Dump:\n"
                  << "  -> Intercepted A_client: " << bn_to_hex(A_client) << "\n"
                  << "  -> Client M1:            " << to_hex(client_M1, 20) << "\n"
                  << "  -> Proxy Expected M1:    " << to_hex(expected_M1, 20) << "\n"
                  << "  -> Generated K_client:   " << to_hex(K_client, 40) << std::endl;

        if (std::memcmp(client_M1, expected_M1, 20) != 0) {
            std::cerr << "[MITM] Client M1 verification FAILED! Mismatch detected." << std::endl;
        } else {
            std::cout << "[MITM] Client M1 verification PASSED!" << std::endl;
        }
        compute_M2(A_client_net, expected_M1, K_client, M2_client);

        a_proxy = BN_new(); BN_rand(a_proxy, 256, -1, 0);
        A_proxy = BN_new();
        // FIX: Switched to le_to_bn
        BIGNUM* bn_N = le_to_bn(N_bytes.data(), N_bytes.size());
        BIGNUM* bn_g = le_to_bn(g_bytes.data(), g_bytes.size());
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
        
        size_t telemetry_offset = 1 + SRP_KEY_SIZE + 20;
        // Always check bounds before accessing data
        size_t bytes_to_copy = 22;
        if (telemetry_offset + bytes_to_copy > len) {
            bytes_to_copy = (len > telemetry_offset) ? (len - telemetry_offset) : 0;
        }
        packet.insert(packet.end(), data + telemetry_offset, data + telemetry_offset + bytes_to_copy);
        return packet;
    }
    
    return std::vector<uint8_t>(data, data + len);
}

std::vector<uint8_t> SRP6Mitm::process_downstream(const uint8_t* data, size_t len) {
    std::lock_guard<std::mutex> lock(mtx);
    if (len == 0) return {};

    uint8_t opcode = data[0];
    std::cout << "[TRACE][DOWNSTREAM] Server -> Client | Opcode: 0x" 
              << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(opcode) 
              << std::dec << " | Length: " << len << " | Current State: " << static_cast<int>(state) << std::endl;

    if (state == MitmState::WAITING_SERVER_CHALLENGE) {
        if (len < 40) return {};
        uint8_t error = data[2];
        if (error != 0) return std::vector<uint8_t>(data, data + len);

        size_t offset = 3;
        B_server = le_to_bn(data + offset, SRP_KEY_SIZE); offset += SRP_KEY_SIZE;
        uint8_t g_len = data[offset++]; g_bytes.assign(data + offset, data + offset + g_len); offset += g_len;
        uint8_t N_len = data[offset++]; N_bytes.assign(data + offset, data + offset + N_len); offset += N_len;
        std::memcpy(salt, data + offset, 32); offset += 32;

        std::cout << "[DEBUG] Downstream Challenge Extracted:\n"
                  << "  -> B_server: " << bn_to_hex(B_server) << "\n"
                  << "  -> salt:     " << to_hex(salt, 32) << "\n"
                  << "  -> g_bytes:  " << to_hex(g_bytes.data(), g_bytes.size()) << "\n"
                  << "  -> N_bytes:  " << to_hex(N_bytes.data(), N_bytes.size()) << std::endl;

        b_proxy = BN_new(); BN_rand(b_proxy, 256, -1, 0);
        B_proxy = BN_new();
        
        // FIX: Switched to le_to_bn
        BIGNUM* bn_N = le_to_bn(N_bytes.data(), N_bytes.size());
        BIGNUM* bn_g = le_to_bn(g_bytes.data(), g_bytes.size());
        BN_CTX* ctx = BN_CTX_new();

        std::string upper_user = username;
        std::transform(upper_user.begin(), upper_user.end(), upper_user.begin(), ::toupper);
        std::string upper_pass = password;
        std::transform(upper_pass.begin(), upper_pass.end(), upper_pass.begin(), ::toupper);

        std::string user_pass = upper_user + ":" + upper_pass;
        uint8_t hash_up[20]; sha1((const uint8_t*)user_pass.c_str(), user_pass.length(), hash_up);
        SHA_CTX x_ctx; SHA1_Init(&x_ctx); SHA1_Update(&x_ctx, salt, 32); SHA1_Update(&x_ctx, hash_up, 20);
        uint8_t x_hash[20]; SHA1_Final(x_hash, &x_ctx);
        
        uint8_t x_le[20];
for (int i = 0; i < 20; i++) x_le[i] = x_hash[19 - i]; // Reverse for LE
BIGNUM* x = BN_bin2bn(x_le, 20, nullptr); 
        BIGNUM* v = BN_new(); BN_mod_exp(v, bn_g, x, bn_N, ctx);

        BIGNUM* bn_k = BN_new(); BN_set_word(bn_k, 3);
        BIGNUM* gb = BN_new(); BN_mod_exp(gb, bn_g, b_proxy, bn_N, ctx);
        BIGNUM* kv = BN_new(); BN_mod_mul(kv, bn_k, v, bn_N, ctx);
        BN_mod_add(B_proxy, kv, gb, bn_N, ctx);
        B_proxy_raw.resize(SRP_KEY_SIZE);
        bn_to_le(B_proxy, B_proxy_raw.data(), SRP_KEY_SIZE); 

        std::cout << "[DEBUG] B_proxy_raw captured: " << to_hex(B_proxy_raw.data(), SRP_KEY_SIZE) << std::endl;
        std::cout << "[DEBUG] Generated Proxy Credentials:\n"
                  << "  -> B_proxy:  " << bn_to_hex(B_proxy) << std::endl;

        BN_clear_free(x); BN_clear_free(v); BN_clear_free(bn_k); BN_clear_free(gb); BN_clear_free(kv);
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
        
        // Clone the original server packet
        std::vector<uint8_t> packet(data, data + len);
        
        // Patch the server's M2 with your locally generated M2_client
        // WoW server proofs usually start at offset 2 (Opcode=1, Error=0, M2=20 bytes)
        std::memcpy(packet.data() + 2, M2_client, 20);
        
        state = MitmState::AUTH_COMPLETE;
        return packet;
    }

    return std::vector<uint8_t>(data, data + len);
}

bool SRP6Mitm::is_auth_complete() { std::lock_guard<std::mutex> lock(mtx); return state == MitmState::AUTH_COMPLETE; }

void SRP6Mitm::get_session_keys(uint8_t* out_client, uint8_t* out_server) {
    std::lock_guard<std::mutex> lock(mtx);
    std::memcpy(out_client, K_client, 40); std::memcpy(out_server, K_server, 40);
}