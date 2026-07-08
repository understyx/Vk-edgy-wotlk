#ifndef PROXY_HPP
#define PROXY_HPP

#include "Protocol.hpp"
#include "RingBuffer.hpp"
#include "SRP6Mitm.hpp"
#include <memory>

size_t get_dynamic_auth_size(const char* header_accumulator, size_t available_len, StreamDirection direction, bool& out_error);
void initialize_session_crypto(SessionContext& context, const uint8_t* raw_session_key);
void writer_thread(int to_fd, RingBuffer& rb, std::shared_ptr<SessionContext> context, std::shared_ptr<SRP6Mitm> mitm_engine, DirectionConfig config, StreamDirection direction);
void bridge(int from_fd, int to_fd, RingBuffer& rb, std::shared_ptr<SessionContext> context, std::shared_ptr<SRP6Mitm> mitm_engine, DirectionConfig config, StreamDirection direction);
void handle_client(int client_fd, ProxyMapping mapping);
void start_listener(ProxyMapping mapping);

#endif // PROXY_HPP
