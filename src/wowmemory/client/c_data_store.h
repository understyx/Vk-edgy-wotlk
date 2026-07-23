#ifndef WOWMEMORY_CLIENT_C_DATA_STORE_H
#define WOWMEMORY_CLIENT_C_DATA_STORE_H

#include <cstdint>
#include <string>
#include <vector>

namespace WoWMemory {

class CDataStore {
public:
    CDataStore();
    CDataStore(uint32_t msgOpcode);
    CDataStore(void* pointer);
    ~CDataStore();

    void* GetPointer() const;

    void* GetData() const;
    void SetData(void* data);

    uint32_t GetBase() const;
    void SetBase(uint32_t baseVal);

    uint32_t GetCapacity() const;
    void SetCapacity(uint32_t cap);

    uint32_t GetBytesWritten() const;
    void SetBytesWritten(uint32_t written);

    uint32_t GetBytesRead() const;
    void SetBytesRead(uint32_t read);

    uint32_t GetOpCode() const;
    bool IsFinal() const;

    // Read / Write Helpers
    uint8_t GetInt8();
    uint16_t GetInt16();
    uint32_t GetInt32();
    uint64_t GetInt64();
    float GetFloat();
    std::string GetString(int length = 256);
    std::vector<uint8_t> GetBytes(int count);

    void PutInt8(uint8_t value);
    void PutInt16(uint16_t value);
    void PutInt32(uint32_t value);
    void PutInt64(uint64_t value);
    void PutFloat(float value);
    void PutString(const std::string& value);
    void PutBytes(const uint8_t* buffer, uint32_t size);

    void Prepare();
    void Destroy();

private:
    void* m_pointer = nullptr;
    bool m_deallocate = false;
#ifndef _WIN32
    // Dummy buffer for host stubs
    std::vector<uint8_t> m_hostBuffer;
    uint32_t m_hostBytesRead = 0;
    uint32_t m_hostOpcode = 0;
#endif
};

} // namespace WoWMemory

#endif // WOWMEMORY_CLIENT_C_DATA_STORE_H
