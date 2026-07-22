#include "wowmemory/memory_utils.h"
#include <unistd.h>
#include <sys/mman.h>

namespace WoWMemory {

bool IsReadableRange(uintptr_t addr, size_t size)
{
    if (size == 0) return true;
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
}

std::string ReadSafeString(uintptr_t startAddr, size_t maxLen)
{
    if (!startAddr) return {};
    static const long page_size = sysconf(_SC_PAGESIZE);

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
