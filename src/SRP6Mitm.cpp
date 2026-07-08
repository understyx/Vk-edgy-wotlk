#include "SRP6Mitm.hpp"
#include <iostream>
#include <cstring>
#include <algorithm>
#include <openssl/sha.h>
#include <openssl/bn.h>

SRP6Mitm::SRP6Mitm(const std::string& user, const std::string& pass) : username(user), password(pass) {}

SRP6Mitm::~SRP6Mitm() {
    if (B_server) BN_clear_free(B_server);
    if (B_proxy) BN_clear_free(B_proxy);
    if (A_client) BN_clear_free(A_client);
    if (A_proxy) BN_clear_free(A_proxy);
    if (a_proxy) BN_clear_free(a_proxy);
    if (b_proxy) BN_clear_free(b_proxy);
}

void SRP6Mitm::sha1(const uint8_t* data, size_t len, uint8_t* out) { SHA1(data, len, out); }

void SRP6Mitm::bn_to_le(const BIGNUM* bn, uint8_t* out, size_t len) {
    std::memset(out, 0, len);
    int size = BN_num_bytes(bn);
    if (size <= 0) return;
    std::vector<uint8_t> temp(size);
    BN_bn2bin(bn, temp.data());
    for (int i = 0; i < size && i < (int)len; ++i) out[i] = temp[size - 1 - i];
}

BIGNUM* SRP6Mitm::le_to_bn(const uint8_t* data, size_t len) {
    std::vector<uint8_t> temp(len);
    for (size_t i = 0; i < len; ++i) temp[i] = data[len - 1 - i];
    return BN_bin2bn(temp.data(), len, nullptr);
}

void SRP6Mitm::compute_session_key(const BIGNUM* S, uint8_t* K) {
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

void SRP6Mitm::compute_M1(const uint8_t* A, const uint8_t* B, const uint8_t* K, uint8_t* M1) {
    uint8_t hash_N[20], hash_g[20], hash_I[20];
    sha1(N_bytes.data(), N_bytes.size(), hash_N);
    sha1(g_bytes.data(), g_bytes.size(), hash_g);

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

void SRP6Mitm::compute_M2(const uint8_t* A, const uint8_t* M1, const uint8_t* K, uint8_t* M2) {
    SHA_CTX ctx; SHA1_Init(&ctx);
    SHA1_Update(&ctx, A, SRP_KEY_SIZE);
    SHA1_Update(&ctx, M1, 20);
    SHA1_Update(&ctx, K, 40);
    SHA1_Final(M2, &ctx);
}

BIGNUM* SRP6Mitm::calculate_S_as_client(const BIGNUM* A, const BIGNUM* B, const BIGNUM* private_key_a) {
    BN_CTX* ctx = BN_CTX_new();
    if (!ctx) return nullptr;
    BIGNUM* bn_N = BN_bin2bn(N_bytes.data(), N_bytes.size(), nullptr);
    BIGNUM* bn_g = BN_bin2bn(g_bytes.data(), g_bytes.size(), nullptr);
    BIGNUM* bn_k = BN_new(); BN_set_word(bn_k, 3);

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

BIGNUM* SRP6Mitm::calculate_S_as_server(const BIGNUM* A_client) {
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

std::vector<uint8_t> SRP6Mitm::process_upstream(const uint8_t* data, size_t len) {
    std::lock_guard<std::mutex> lock(mtx);
    if (state == MitmState::WAITING_CLIENT_CHALLENGE) {
        if (len > 34) {
            uint8_t I_len = data[33];
            if (I_len > 0 && I_len < 100 && (static_cast<size_t>(34) + I_len <= len)) {
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

std::vector<uint8_t> SRP6Mitm::process_downstream(const uint8_t* data, size_t len) {
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

bool SRP6Mitm::is_auth_complete() { std::lock_guard<std::mutex> lock(mtx); return state == MitmState::AUTH_COMPLETE; }

void SRP6Mitm::get_session_keys(uint8_t* out_client, uint8_t* out_server) {
    std::lock_guard<std::mutex> lock(mtx);
    std::memcpy(out_client, K_client, 40); std::memcpy(out_server, K_server, 40);
}
