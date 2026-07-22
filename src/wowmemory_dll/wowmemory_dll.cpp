#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <iostream>

#include "wowmemory/wowmemory.h"

// Link with Ws2_32.lib
#pragma comment(lib, "ws2_32.lib")

static std::atomic<bool> g_Running(false);

static void SocketClientThread()
{
    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return;
    }

    WoWMemory::GameDataReader reader;
    std::vector<uint8_t> serializeBuf;

    while (g_Running) {
        SOCKET connectSocket = INVALID_SOCKET;
        struct sockaddr_in clientService;

        connectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (connectSocket == INVALID_SOCKET) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }

        clientService.sin_family = AF_INET;
        clientService.sin_addr.s_addr = inet_addr("127.0.0.1");
        clientService.sin_port = htons(50055);

        // Try to connect to localhost port 50055
        int connResult = connect(connectSocket, (SOCKADDR*)&clientService, sizeof(clientService));
        if (connResult == SOCKET_ERROR) {
            closesocket(connectSocket);
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }

        // Successfully connected! Enter send loop.
        while (g_Running) {
            WoWMemory::GameData data;
            // Read game memory safely
            reader.ReadGameData(data);

            // Serialize data
            WoWMemory::SerializeGameData(data, serializeBuf);

            // Send length first (framing)
            uint32_t payloadLen = static_cast<uint32_t>(serializeBuf.size());
            int bytesSent = send(connectSocket, reinterpret_cast<const char*>(&payloadLen), sizeof(payloadLen), 0);
            if (bytesSent == SOCKET_ERROR) {
                break;
            }

            // Send actual serialized data payload
            if (payloadLen > 0) {
                bytesSent = send(connectSocket, reinterpret_cast<const char*>(serializeBuf.data()), payloadLen, 0);
                if (bytesSent == SOCKET_ERROR) {
                    break;
                }
            }

            // Sleep ~30ms to maintain ~33 updates per second (perfect overlay fluidity without taxing CPU)
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
        }

        closesocket(connectSocket);
    }

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
        DisableThreadLibraryCalls(hModule);
        g_Running = true;

        CreateThread(
            nullptr,
            0,
            StartThread,
            nullptr,
            0,
            nullptr
        );
    } else if (reason == DLL_PROCESS_DETACH) {
        g_Running = false;
        // Since the DLL is detaching/unloading, we let the thread exit on g_Running = false.
        // We do not wait/join inside DLL_PROCESS_DETACH to avoid deadlocking with thread exit under loader lock.
    }
    return TRUE;
}
