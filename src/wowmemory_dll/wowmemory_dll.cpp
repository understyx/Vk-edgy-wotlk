#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <iostream>
#include <string>
#include <limits>

#include "wowmemory/client_profile.h"
#include "wowmemory/wowmemory.h"

// Link with Ws2_32.lib
#pragma comment(lib, "ws2_32.lib")

static std::atomic<bool> g_Running(false);

static void LogMsg(const std::string& msg)
{
    std::cerr << "[wowmemory.dll] " << msg << std::endl;
    FILE* f = fopen("wow_ipc_debug.log", "a");
    if (f) {
        fprintf(f, "[wowmemory.dll] %s\n", msg.c_str());
        fclose(f);
    }
}

static bool SendExactly(SOCKET socket, const void* data, size_t size)
{
    const char* cursor = reinterpret_cast<const char*>(data);
    size_t remaining = size;
    while (remaining > 0) {
        const size_t chunkSize = (remaining > static_cast<size_t>(std::numeric_limits<int>::max()))
            ? static_cast<size_t>(std::numeric_limits<int>::max())
            : remaining;
        const int sent = send(socket, cursor, static_cast<int>(chunkSize), 0);
        if (sent == SOCKET_ERROR || sent == 0) {
            return false;
        }
        cursor += sent;
        remaining -= static_cast<size_t>(sent);
    }
    return true;
}

static void SocketClientThread()
{
    LogMsg("Worker thread started.");

    const WoWMemory::ClientProfileStatus& profile = WoWMemory::GetClientProfileStatus();
    LogMsg(profile.message);
    if (!profile.supported) {
        LogMsg("Memory reading disabled to prevent using absolute offsets with a different client build.");
        g_Running = false;
        return;
    }

    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        LogMsg("WSAStartup failed.");
        return;
    }

    LogMsg("WSAStartup succeeded.");

    WoWMemory::GameDataReader reader;
    std::vector<uint8_t> serializeBuf;

    uint32_t totalPacketsSent = 0;

    while (g_Running) {
        SOCKET connectSocket = INVALID_SOCKET;
        struct sockaddr_in clientService;

        connectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (connectSocket == INVALID_SOCKET) {
            LogMsg("Socket creation failed.");
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }

        clientService.sin_family = AF_INET;
        clientService.sin_addr.s_addr = inet_addr("127.0.0.1");
        clientService.sin_port = htons(50055);

        LogMsg("Attempting to connect to host Vulkan layer (127.0.0.1:50055)...");

        // Try to connect to localhost port 50055
        int connResult = connect(connectSocket, (SOCKADDR*)&clientService, sizeof(clientService));
        if (connResult == SOCKET_ERROR) {
            closesocket(connectSocket);
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }

        LogMsg("Successfully connected to host Vulkan layer!");

        // Successfully connected! Enter send loop.
        while (g_Running) {
            WoWMemory::GameData data;
            // Read game memory safely
            if (!reader.ReadGameData(data)) {
                LogMsg("Client profile validation failed; stopping the reader.");
                g_Running = false;
                break;
            }

            // Serialize data
            WoWMemory::SerializeGameData(data, serializeBuf);

            // Send length first (framing)
            uint32_t payloadLen = static_cast<uint32_t>(serializeBuf.size());
            if (!SendExactly(connectSocket, &payloadLen, sizeof(payloadLen))) {
                LogMsg("Send framing length failed. Connection likely closed.");
                break;
            }

            // Send actual serialized data payload
            if (payloadLen > 0) {
                if (!SendExactly(connectSocket, serializeBuf.data(), payloadLen)) {
                    LogMsg("Send payload failed. Connection likely closed.");
                    break;
                }
            }

            totalPacketsSent++;
            if (totalPacketsSent % 200 == 1) {
                LogMsg("Active sending data packets (total packets sent so far: " + std::to_string(totalPacketsSent) + ", last payload size: " + std::to_string(payloadLen) + " bytes).");
            }

            // Sleep ~30ms to maintain ~33 updates per second (perfect overlay fluidity without taxing CPU)
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
        }

        LogMsg("Connection closed or loop interrupted. Cleaning up socket...");
        closesocket(connectSocket);
    }

    LogMsg("Worker thread terminating.");
    WSACleanup();
}

static DWORD WINAPI StartThread(LPVOID)
{
    SocketClientThread();
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH) {
        LogMsg("DllMain: DLL_PROCESS_ATTACH received.");
        DisableThreadLibraryCalls(hModule);
        g_Running = true;

        HANDLE hThread = CreateThread(
            nullptr,
            0,
            StartThread,
            nullptr,
            0,
            nullptr
        );
        if (hThread) {
            LogMsg("DllMain: CreateThread succeeded.");
            CloseHandle(hThread);
        } else {
            LogMsg("DllMain: CreateThread failed!");
        }
    } else if (reason == DLL_PROCESS_DETACH) {
        LogMsg("DllMain: DLL_PROCESS_DETACH received.");
        g_Running = false;
        // Since the DLL is detaching/unloading, we let the thread exit on g_Running = false.
        // We do not wait/join inside DLL_PROCESS_DETACH to avoid deadlocking with thread exit under loader lock.
    }
    return TRUE;
}
