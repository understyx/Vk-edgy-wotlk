#ifndef WOWMEMORY_MEMORY_UTILS_H
#define WOWMEMORY_MEMORY_UTILS_H

#include <cstdint>
#include <string>

namespace WoWMemory {

/**
 * @brief Safely check if a memory range is mapped and readable.
 * @param addr The start address to check.
 * @param size The size of the memory range.
 * @return true if readable, false otherwise.
 */
bool IsReadableRange(uintptr_t addr, size_t size);

/**
 * @brief Safely cast an absolute 32-bit address to a typed pointer.
 */
template<typename T>
const T* AbsPtr(uint32_t addr) {
    return reinterpret_cast<const T*>(static_cast<uintptr_t>(addr));
}

/**
 * @brief Safely read a value from an absolute address.
 */
template<typename T>
T ReadAbs(uint32_t addr, T fallback = T{}) {
    if (!IsReadableRange(static_cast<uintptr_t>(addr), sizeof(T))) {
        return fallback;
    }
    return *AbsPtr<T>(addr);
}

/**
 * @brief Safely read a string up to maxLen, checking each page's readability first.
 */
std::string ReadSafeString(uintptr_t startAddr, size_t maxLen);

/**
 * @brief Read a bounded null-terminated string directly from an absolute address.
 */
std::string ReadInlineString(uint32_t absAddr, size_t maxLen = 64);

/**
 * @brief Read a null-terminated string through a char* pointer stored at absAddr.
 */
std::string ReadIndirectString(uint32_t ptrAddr, size_t maxLen = 64);

} // namespace WoWMemory

#endif // WOWMEMORY_MEMORY_UTILS_H
