#include "Proxy.hpp"
#include <iostream>
#include <vector>
#include <thread>
#include <sstream>

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

        mapping.upstream = { 6, 4, false };
        mapping.downstream = { 4, 2, true };

        if (parts.size() >= 4 && parts[3] == "a") mapping.initial_mode = ProxyMode::WOW_AUTH;
        if (parts.size() >= 6) { mapping.auth_user = parts[4]; mapping.auth_pass = parts[5]; }

        listeners.emplace_back(start_listener, mapping);
    }
    for (auto& t : listeners) if (t.joinable()) t.join();
    return 0;
}