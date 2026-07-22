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

static bool InjectDLL(DWORD pid, const std::string& dllPath)
{
    HANDLE processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!processHandle) {
        std::cerr << "Failed to open target process. Error: " << GetLastError() << "\n";
        return false;
    }

    LPVOID remoteBuffer = VirtualAllocEx(processHandle, NULL, dllPath.size() + 1, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remoteBuffer) {
        std::cerr << "Failed to allocate memory in target process. Error: " << GetLastError() << "\n";
        CloseHandle(processHandle);
        return false;
    }

    if (!WriteProcessMemory(processHandle, remoteBuffer, dllPath.c_str(), dllPath.size() + 1, NULL)) {
        std::cerr << "Failed to write to target process memory. Error: " << GetLastError() << "\n";
        VirtualFreeEx(processHandle, remoteBuffer, 0, MEM_RELEASE);
        CloseHandle(processHandle);
        return false;
    }

    LPVOID loadLibraryAddress = (LPVOID)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");
    if (!loadLibraryAddress) {
        std::cerr << "Failed to get LoadLibraryA address.\n";
        VirtualFreeEx(processHandle, remoteBuffer, 0, MEM_RELEASE);
        CloseHandle(processHandle);
        return false;
    }

    HANDLE remoteThread = CreateRemoteThread(processHandle, NULL, 0, (LPTHREAD_START_ROUTINE)loadLibraryAddress, remoteBuffer, 0, NULL);
    if (!remoteThread) {
        std::cerr << "Failed to create remote thread. Error: " << GetLastError() << "\n";
        VirtualFreeEx(processHandle, remoteBuffer, 0, MEM_RELEASE);
        CloseHandle(processHandle);
        return false;
    }

    WaitForSingleObject(remoteThread, INFINITE);
    CloseHandle(remoteThread);
    VirtualFreeEx(processHandle, remoteBuffer, 0, MEM_RELEASE);
    CloseHandle(processHandle);
    return true;
}

int main()
{
    std::cout << "Starting DLL Injector...\n";

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
        std::cerr << "Could not find WoW.exe process.\n";
        return 1;
    }

    std::cout << "Found WoW.exe process with PID: " << pid << "\n";

    // Resolve absolute path to wowmemory.dll in the same directory
    char dllPath[MAX_PATH];
    GetModuleFileNameA(NULL, dllPath, MAX_PATH);
    char* lastSlash = strrchr(dllPath, '\\');
    if (lastSlash) {
        strcpy(lastSlash + 1, "wowmemory.dll");
    } else {
        strcpy(dllPath, "wowmemory.dll");
    }

    std::cout << "Injecting DLL: " << dllPath << "\n";

    if (InjectDLL(pid, dllPath)) {
        std::cout << "Successfully injected wowmemory.dll into WoW.exe!\n";
        return 0;
    } else {
        std::cerr << "DLL injection failed.\n";
        return 1;
    }
}
