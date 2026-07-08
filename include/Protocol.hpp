#ifndef PROTOCOL_HPP
#define PROTOCOL_HPP

#include <string>
#include <mutex>
#include <cstring>
#include <openssl/rc4.h>

const size_t RING_BUFFER_SIZE = 512 * 1024;

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

#endif // PROTOCOL_HPP
