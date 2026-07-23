#include "helper.h"
#include "wowmemory/offsets.h"
#include "wowmemory/memory_utils.h"

namespace WoWMemory {

uint32_t Helper::PerformanceCount() {
#ifdef _WIN32
    typedef uint32_t (__cdecl *PerformanceCounter_t)();
    auto fn = reinterpret_cast<PerformanceCounter_t>(WoWOffsets::Other::PerformanceCounter);
    return fn();
#else
    return 0;
#endif
}

void Helper::ResetHardwareAction() {
#ifdef _WIN32
    if (IsReadableRange(WoWOffsets::Other::LastHardwareAction, sizeof(uint32_t))) {
        *reinterpret_cast<uint32_t*>(WoWOffsets::Other::LastHardwareAction) = PerformanceCount();
    }
#endif
}

void Helper::FixInvalidPtrCheck() {
#ifdef _WIN32
    if (IsReadableRange(WoWOffsets::Console::InvalidPtrCheck, sizeof(uint32_t) * 2)) {
        *reinterpret_cast<uint32_t*>(WoWOffsets::Console::InvalidPtrCheck) = 0x00000001;
        *reinterpret_cast<uint32_t*>(WoWOffsets::Console::InvalidPtrCheck + 0x4) = 0x7FFFFFFF;
    }
#endif
}

bool Helper::InCombat() {
    // This is typically parsed via client event listeners; we keep a passive stub.
    return false;
}

} // namespace WoWMemory
