#include "wowmemory/client_profile.h"

#include <cstdio>

#ifdef _WIN32
#include <windows.h>
#endif

namespace WoWMemory {
namespace {

ClientProfileStatus InspectClientProfile()
{
    ClientProfileStatus status;

#ifdef _WIN32
    const HMODULE module = GetModuleHandleW(nullptr);
    status.moduleBase = reinterpret_cast<uintptr_t>(module);
    if (!module) {
        status.message = "unsupported client: GetModuleHandleW(nullptr) failed";
        return status;
    }

    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(module);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew <= 0 || dos->e_lfanew > 0x1000) {
        status.message = "unsupported client: invalid DOS header";
        return status;
    }

    const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS32*>(
        reinterpret_cast<const uint8_t*>(module) + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE ||
        nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
        status.message = "unsupported client: executable is not PE32";
        return status;
    }

    status.machine = nt->FileHeader.Machine;
    status.timestamp = nt->FileHeader.TimeDateStamp;
    status.imageSize = nt->OptionalHeader.SizeOfImage;
    status.entryPointRva = nt->OptionalHeader.AddressOfEntryPoint;

    const bool matches =
        status.moduleBase == kSupportedClientImageBase &&
        status.machine == kSupportedClientMachine &&
        status.timestamp == kSupportedClientTimestamp &&
        nt->OptionalHeader.ImageBase == kSupportedClientImageBase &&
        status.imageSize == kSupportedClientImageSize &&
        status.entryPointRva == kSupportedClientEntryPointRva;

    char details[320];
    std::snprintf(details, sizeof(details),
                  "%s WoW 3.3.5a/12340 profile: base=0x%08llX machine=0x%04X "
                  "timestamp=0x%08X imageSize=0x%08X entryRva=0x%08X",
                  matches ? "validated" : "unsupported",
                  static_cast<unsigned long long>(status.moduleBase),
                  static_cast<unsigned>(status.machine), status.timestamp,
                  status.imageSize, status.entryPointRva);
    status.supported = matches;
    status.message = details;
#else
    status.message = "client profile validation is only applicable inside the Windows reader DLL";
#endif

    return status;
}

} // namespace

const ClientProfileStatus& GetClientProfileStatus()
{
    static const ClientProfileStatus status = InspectClientProfile();
    return status;
}

} // namespace WoWMemory
