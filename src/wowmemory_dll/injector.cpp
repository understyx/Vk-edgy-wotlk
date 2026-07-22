#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <string>

static DWORD GetProcessIdByName(const std::string& processName)
{
    DWORD pid = 0;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 processEntry;
        processEntry.dwSize = sizeof(processEntry);
        if (Process32First(snapshot, &processEntry)) {
            do {
                if (processName == processEntry.szExeFile) {
                    pid = processEntry.th32ProcessID;
                    break;
                }
            } while (Process32Next(snapshot, &processEntry));
        }
        CloseHandle(snapshot);
    }
    return pid;
}

static void LogMsg(const std::string& msg)
{
    std::cerr << "[Injector] " << msg << std::endl;
    FILE* f = fopen("wow_ipc_debug.log", "a");
    if (f) {
        fprintf(f, "[Injector] %s\n", msg.c_str());
        fclose(f);
    }
}

static bool InjectDLL(DWORD pid, const std::string& dllPath)
{
    LogMsg("Attempting to inject DLL: " + dllPath + " into PID: " + std::to_string(pid));
    HANDLE processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!processHandle) {
        LogMsg("Failed to open target process. Error: " + std::to_string(GetLastError()));
        return false;
    }

    LPVOID remoteBuffer = VirtualAllocEx(processHandle, NULL, dllPath.size() + 1, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remoteBuffer) {
        LogMsg("Failed to allocate memory in target process. Error: " + std::to_string(GetLastError()));
        CloseHandle(processHandle);
        return false;
    }

    if (!WriteProcessMemory(processHandle, remoteBuffer, dllPath.c_str(), dllPath.size() + 1, NULL)) {
        LogMsg("Failed to write to target process memory. Error: " + std::to_string(GetLastError()));
        VirtualFreeEx(processHandle, remoteBuffer, 0, MEM_RELEASE);
        CloseHandle(processHandle);
        return false;
    }

    LPVOID loadLibraryAddress = (LPVOID)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");
    if (!loadLibraryAddress) {
        LogMsg("Failed to get LoadLibraryA address.");
        VirtualFreeEx(processHandle, remoteBuffer, 0, MEM_RELEASE);
        CloseHandle(processHandle);
        return false;
    }

    HANDLE remoteThread = CreateRemoteThread(processHandle, NULL, 0, (LPTHREAD_START_ROUTINE)loadLibraryAddress, remoteBuffer, 0, NULL);
    if (!remoteThread) {
        LogMsg("Failed to create remote thread. Error: " + std::to_string(GetLastError()));
        VirtualFreeEx(processHandle, remoteBuffer, 0, MEM_RELEASE);
        CloseHandle(processHandle);
        return false;
    }

    WaitForSingleObject(remoteThread, INFINITE);

    // Check if LoadLibrary succeeded in the remote process by getting thread exit code
    DWORD exitCode = 0;
    if (GetExitCodeThread(remoteThread, &exitCode)) {
        LogMsg("Remote LoadLibrary returned exit code: " + std::to_string(exitCode) + " (non-zero means success)");
    }

    CloseHandle(remoteThread);
    VirtualFreeEx(processHandle, remoteBuffer, 0, MEM_RELEASE);
    CloseHandle(processHandle);
    return exitCode != 0;
}

int main()
{
    LogMsg("Starting DLL Injector...");

    // Wait up to 30 seconds for WoW.exe to launch
    DWORD pid = 0;
    for (int i = 0; i < 30; ++i) {
        pid = GetProcessIdByName("WoW.exe");
        if (pid == 0) {
            pid = GetProcessIdByName("wow.exe");
        }
        if (pid != 0) {
            break;
        }
        Sleep(1000);
    }

    if (pid == 0) {
        LogMsg("Could not find WoW.exe process within 30 seconds.");
        return 1;
    }

    LogMsg("Found WoW.exe process with PID: " + std::to_string(pid));

    // Resolve absolute path to wowmemory.dll in the same directory
    char dllPath[MAX_PATH];
    GetModuleFileNameA(NULL, dllPath, MAX_PATH);
    char* lastSlash = strrchr(dllPath, '\\');
    if (lastSlash) {
        strcpy(lastSlash + 1, "wowmemory.dll");
    } else {
        strcpy(dllPath, "wowmemory.dll");
    }

    LogMsg("Resolved injection DLL path: " + std::string(dllPath));

    if (InjectDLL(pid, dllPath)) {
        LogMsg("Successfully injected wowmemory.dll into WoW.exe!");
        return 0;
    } else {
        LogMsg("DLL injection failed.");
        return 1;
    }
}
