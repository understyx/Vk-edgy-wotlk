#include "wowmemory/memory_utils.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/mman.h>
#endif

namespace WoWMemory {

bool IsReadableRange(uintptr_t addr, size_t size)
{
    if (size == 0) return true;

#ifdef _WIN32
    // Windows implementation using VirtualQuery
    MEMORY_BASIC_INFORMATION mbi;
    uintptr_t current = addr;
    uintptr_t end = addr + size;
    while (current < end) {
        if (VirtualQuery(reinterpret_cast<LPCVOID>(current), &mbi, sizeof(mbi)) == 0) {
            return false;
        }
        // Check if the page is committed and is readable (not PAGE_NOACCESS or PAGE_GUARD)
        if (mbi.State != MEM_COMMIT) {
            return false;
        }
        if (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) {
            return false;
        }
        // Move to the next region
        uintptr_t next_region = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
        if (next_region <= current) {
            // Prevent infinite loop if overflow or RegionSize is 0
            break;
        }
        current = next_region;
    }
    return true;
#else
    static const long page_size = sysconf(_SC_PAGESIZE);

    uintptr_t start = addr;
    uintptr_t end = start + size;

    uintptr_t first_page = start & ~(page_size - 1);
    uintptr_t last_page = (end - 1) & ~(page_size - 1);

    // 1-entry thread_local cache to avoid redundant mincore system calls.
    thread_local uintptr_t s_last_page = 0;
    thread_local bool s_last_readable = false;

    for (uintptr_t p = first_page; p <= last_page; p += page_size) {
        if (p == s_last_page) {
            if (!s_last_readable) return false;
            continue;
        }

        unsigned char vec;
        bool readable = (mincore(reinterpret_cast<void*>(p), page_size, &vec) == 0);
        s_last_page = p;
        s_last_readable = readable;

        if (!readable) return false;
    }
    return true;
#endif
}

std::string ReadSafeString(uintptr_t startAddr, size_t maxLen)
{
    if (!startAddr) return {};

#ifdef _WIN32
    // Simple page-safe check for Windows. We get the system page size first.
    static size_t page_size = 0;
    if (page_size == 0) {
        SYSTEM_INFO si;
        GetSystemInfo(&si);
        page_size = si.dwPageSize;
    }
#else
    static const long page_size = sysconf(_SC_PAGESIZE);
#endif

    uintptr_t current = startAddr;
    size_t len = 0;

    uintptr_t current_page = 0;
    bool current_page_readable = false;

    while (len < maxLen) {
        uintptr_t page = current & ~(page_size - 1);
        if (page != current_page) {
            current_page = page;
            current_page_readable = IsReadableRange(current_page, page_size);
        }

        if (!current_page_readable) {
            break;
        }

        const char* p = reinterpret_cast<const char*>(current);
        if (*p == '\0') {
            break;
        }

        current++;
        len++;
    }

    if (len == 0) return {};
    return std::string(reinterpret_cast<const char*>(startAddr), len);
}

std::string ReadInlineString(uint32_t absAddr, size_t maxLen)
{
    return ReadSafeString(static_cast<uintptr_t>(absAddr), maxLen);
}

std::string ReadIndirectString(uint32_t ptrAddr, size_t maxLen)
{
    if (!ptrAddr) return {};
    uintptr_t strPtr = ReadAbs<uint32_t>(ptrAddr);
    if (!strPtr) return {};
    return ReadSafeString(strPtr, maxLen);
}

} // namespace WoWMemory
