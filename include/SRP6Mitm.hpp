#ifndef SRP6MITM_HPP
#define SRP6MITM_HPP

#include <string>
#include <vector>
#include <mutex>
#include <cstdint>
#include <openssl/bn.h>
#include <openssl/sha.h>

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

    void sha1(const uint8_t* data, size_t len, uint8_t* out);
    void bn_to_le(const BIGNUM* bn, uint8_t* out, size_t len);
    BIGNUM* le_to_bn(const uint8_t* data, size_t len);
    void compute_session_key(const BIGNUM* S, uint8_t* K);
    void compute_M1(const uint8_t* A, const uint8_t* B, const uint8_t* K, uint8_t* M1);
    void compute_M2(const uint8_t* A, const uint8_t* M1, const uint8_t* K, uint8_t* M2);
    BIGNUM* calculate_S_as_client(const BIGNUM* A, const BIGNUM* B, const BIGNUM* private_key_a);
    BIGNUM* calculate_S_as_server(const BIGNUM* A_client);

public:
    SRP6Mitm(const std::string& user, const std::string& pass);
    ~SRP6Mitm();

    std::vector<uint8_t> process_upstream(const uint8_t* data, size_t len);
    std::vector<uint8_t> process_downstream(const uint8_t* data, size_t len);

    bool is_auth_complete();
    void get_session_keys(uint8_t* out_client, uint8_t* out_server);
};

#endif // SRP6MITM_HPP
