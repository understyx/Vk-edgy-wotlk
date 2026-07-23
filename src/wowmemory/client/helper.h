#ifndef WOWMEMORY_CLIENT_HELPER_H
#define WOWMEMORY_CLIENT_HELPER_H

#include <cstdint>

namespace WoWMemory {

class Helper {
public:
    static uint32_t PerformanceCount();
    static void ResetHardwareAction();
    static void FixInvalidPtrCheck();
    static bool InCombat();
};

} // namespace WoWMemory

#endif // WOWMEMORY_CLIENT_HELPER_H
