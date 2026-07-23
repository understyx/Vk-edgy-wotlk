#include "c_data_store.h"
#include "wowmemory/offsets.h"
#include "wowmemory/memory_utils.h"

#ifdef _WIN32
#include <windows.h>
#endif
#include <cstring>

namespace WoWMemory {

CDataStore::CDataStore() {
#ifdef _WIN32
    m_deallocate = true;
    typedef void* (__thiscall *Initialize_t)(void*);
    auto init = reinterpret_cast<Initialize_t>(WoWOffsets::Packets::Initialize);
    m_pointer = malloc(0x18);
    if (m_pointer) {
        init(m_pointer);
    }
#else
    m_deallocate = true;
    m_pointer = this;
#endif
}

CDataStore::CDataStore(uint32_t msgOpcode) : CDataStore() {
    PutInt32(msgOpcode);
#ifndef _WIN32
    m_hostOpcode = msgOpcode;
#endif
}

CDataStore::CDataStore(void* pointer) {
    m_pointer = pointer;
    m_deallocate = false;
}

CDataStore::~CDataStore() {
    if (m_deallocate && m_pointer) {
        Destroy();
#ifdef _WIN32
        free(m_pointer);
#endif
        m_pointer = nullptr;
    }
}

void* CDataStore::GetPointer() const {
    return m_pointer;
}

void* CDataStore::GetData() const {
#ifdef _WIN32
    if (!m_pointer) return nullptr;
    return *reinterpret_cast<void**>(static_cast<char*>(m_pointer) + 0x4);
#else
    return nullptr;
#endif
}

void CDataStore::SetData(void* data) {
#ifdef _WIN32
    if (!m_pointer) return;
    *reinterpret_cast<void**>(static_cast<char*>(m_pointer) + 0x4) = data;
#endif
}

uint32_t CDataStore::GetBase() const {
#ifdef _WIN32
    if (!m_pointer) return 0;
    return *reinterpret_cast<uint32_t*>(static_cast<char*>(m_pointer) + 0x8);
#else
    return 0;
#endif
}

void CDataStore::SetBase(uint32_t baseVal) {
#ifdef _WIN32
    if (!m_pointer) return;
    *reinterpret_cast<uint32_t*>(static_cast<char*>(m_pointer) + 0x8) = baseVal;
#endif
}

uint32_t CDataStore::GetCapacity() const {
#ifdef _WIN32
    if (!m_pointer) return 0;
    return *reinterpret_cast<uint32_t*>(static_cast<char*>(m_pointer) + 0xC);
#else
    return static_cast<uint32_t>(m_hostBuffer.size());
#endif
}

void CDataStore::SetCapacity(uint32_t cap) {
#ifdef _WIN32
    if (!m_pointer) return;
    *reinterpret_cast<uint32_t*>(static_cast<char*>(m_pointer) + 0xC) = cap;
#endif
}

uint32_t CDataStore::GetBytesWritten() const {
#ifdef _WIN32
    if (!m_pointer) return 0;
    return *reinterpret_cast<uint32_t*>(static_cast<char*>(m_pointer) + 0x10);
#else
    return static_cast<uint32_t>(m_hostBuffer.size());
#endif
}

void CDataStore::SetBytesWritten(uint32_t written) {
#ifdef _WIN32
    if (!m_pointer) return;
    *reinterpret_cast<uint32_t*>(static_cast<char*>(m_pointer) + 0x10) = written;
#endif
}

uint32_t CDataStore::GetBytesRead() const {
#ifdef _WIN32
    if (!m_pointer) return 0;
    return *reinterpret_cast<uint32_t*>(static_cast<char*>(m_pointer) + 0x14);
#else
    return m_hostBytesRead;
#endif
}

void CDataStore::SetBytesRead(uint32_t read) {
#ifdef _WIN32
    if (!m_pointer) return;
    *reinterpret_cast<uint32_t*>(static_cast<char*>(m_pointer) + 0x14) = read;
#else
    m_hostBytesRead = read;
#endif
}

uint32_t CDataStore::GetOpCode() const {
#ifdef _WIN32
    void* data = GetData();
    if (!data) return 0;
    return *reinterpret_cast<uint32_t*>(data);
#else
    return m_hostOpcode;
#endif
}

bool CDataStore::IsFinal() const {
    return GetBytesRead() != 0;
}

uint8_t CDataStore::GetInt8() {
#ifdef _WIN32
    if (!m_pointer) return 0;
    typedef void* (__thiscall *GetInt8_t)(void*, uint8_t*);
    auto fn = reinterpret_cast<GetInt8_t>(WoWOffsets::Packets::GetInt8);
    uint8_t val = 0;
    fn(m_pointer, &val);
    return val;
#else
    if (m_hostBytesRead + 1 > m_hostBuffer.size()) return 0;
    uint8_t val = m_hostBuffer[m_hostBytesRead];
    m_hostBytesRead += 1;
    return val;
#endif
}

uint16_t CDataStore::GetInt16() {
#ifdef _WIN32
    if (!m_pointer) return 0;
    typedef void* (__thiscall *GetInt16_t)(void*, uint16_t*);
    auto fn = reinterpret_cast<GetInt16_t>(WoWOffsets::Packets::GetInt16);
    uint16_t val = 0;
    fn(m_pointer, &val);
    return val;
#else
    if (m_hostBytesRead + 2 > m_hostBuffer.size()) return 0;
    uint16_t val;
    std::memcpy(&val, &m_hostBuffer[m_hostBytesRead], 2);
    m_hostBytesRead += 2;
    return val;
#endif
}

uint32_t CDataStore::GetInt32() {
#ifdef _WIN32
    if (!m_pointer) return 0;
    typedef void* (__thiscall *GetInt32_t)(void*, uint32_t*);
    auto fn = reinterpret_cast<GetInt32_t>(WoWOffsets::Packets::GetInt32);
    uint32_t val = 0;
    fn(m_pointer, &val);
    return val;
#else
    if (m_hostBytesRead + 4 > m_hostBuffer.size()) return 0;
    uint32_t val;
    std::memcpy(&val, &m_hostBuffer[m_hostBytesRead], 4);
    m_hostBytesRead += 4;
    return val;
#endif
}

uint64_t CDataStore::GetInt64() {
#ifdef _WIN32
    if (!m_pointer) return 0;
    typedef void* (__thiscall *GetInt64_t)(void*, uint64_t*);
    auto fn = reinterpret_cast<GetInt64_t>(WoWOffsets::Packets::GetInt64);
    uint64_t val = 0;
    fn(m_pointer, &val);
    return val;
#else
    if (m_hostBytesRead + 8 > m_hostBuffer.size()) return 0;
    uint64_t val;
    std::memcpy(&val, &m_hostBuffer[m_hostBytesRead], 8);
    m_hostBytesRead += 8;
    return val;
#endif
}

float CDataStore::GetFloat() {
#ifdef _WIN32
    if (!m_pointer) return 0.0f;
    typedef void* (__thiscall *GetFloat_t)(void*, float*);
    auto fn = reinterpret_cast<GetFloat_t>(WoWOffsets::Packets::GetFloat);
    float val = 0.0f;
    fn(m_pointer, &val);
    return val;
#else
    if (m_hostBytesRead + 4 > m_hostBuffer.size()) return 0.0f;
    float val;
    std::memcpy(&val, &m_hostBuffer[m_hostBytesRead], 4);
    m_hostBytesRead += 4;
    return val;
#endif
}

std::string CDataStore::GetString(int length) {
#ifdef _WIN32
    if (!m_pointer) return "";
    typedef void* (__thiscall *GetString_t)(void*, char*, int);
    auto fn = reinterpret_cast<GetString_t>(WoWOffsets::Packets::GetString);
    char* buf = static_cast<char*>(malloc(length));
    if (!buf) return "";
    fn(m_pointer, buf, length);
    std::string s(buf);
    free(buf);
    return s;
#else
    std::string s;
    while (m_hostBytesRead < m_hostBuffer.size()) {
        char c = static_cast<char>(m_hostBuffer[m_hostBytesRead++]);
        if (c == '\0') break;
        s.push_back(c);
    }
    return s;
#endif
}

std::vector<uint8_t> CDataStore::GetBytes(int count) {
    std::vector<uint8_t> bytes(count, 0);
#ifdef _WIN32
    if (m_pointer) {
        typedef void* (__thiscall *GetBytes_t)(void*, void*, int);
        auto fn = reinterpret_cast<GetBytes_t>(WoWOffsets::Packets::GetBytes);
        fn(m_pointer, bytes.data(), count);
    }
#else
    if (m_hostBytesRead + count <= m_hostBuffer.size()) {
        std::memcpy(bytes.data(), &m_hostBuffer[m_hostBytesRead], count);
        m_hostBytesRead += count;
    }
#endif
    return bytes;
}

void CDataStore::PutInt8(uint8_t value) {
#ifdef _WIN32
    if (!m_pointer) return;
    typedef void* (__thiscall *PutInt8_t)(void*, uint8_t);
    auto fn = reinterpret_cast<PutInt8_t>(WoWOffsets::Packets::PutInt8);
    fn(m_pointer, value);
#else
    m_hostBuffer.push_back(value);
#endif
}

void CDataStore::PutInt16(uint16_t value) {
#ifdef _WIN32
    if (!m_pointer) return;
    typedef void* (__thiscall *PutInt16_t)(void*, uint16_t);
    auto fn = reinterpret_cast<PutInt16_t>(WoWOffsets::Packets::PutInt16);
    fn(m_pointer, value);
#else
    uint8_t buf[2];
    std::memcpy(buf, &value, 2);
    m_hostBuffer.push_back(buf[0]);
    m_hostBuffer.push_back(buf[1]);
#endif
}

void CDataStore::PutInt32(uint32_t value) {
#ifdef _WIN32
    if (!m_pointer) return;
    typedef void* (__thiscall *PutInt32_t)(void*, uint32_t);
    auto fn = reinterpret_cast<PutInt32_t>(WoWOffsets::Packets::PutInt32);
    fn(m_pointer, value);
#else
    uint8_t buf[4];
    std::memcpy(buf, &value, 4);
    for (int i = 0; i < 4; ++i) m_hostBuffer.push_back(buf[i]);
#endif
}

void CDataStore::PutInt64(uint64_t value) {
#ifdef _WIN32
    if (!m_pointer) return;
    typedef void* (__thiscall *PutInt64_t)(void*, uint64_t);
    auto fn = reinterpret_cast<PutInt64_t>(WoWOffsets::Packets::PutInt64);
    fn(m_pointer, value);
#else
    uint8_t buf[8];
    std::memcpy(buf, &value, 8);
    for (int i = 0; i < 8; ++i) m_hostBuffer.push_back(buf[i]);
#endif
}

void CDataStore::PutFloat(float value) {
#ifdef _WIN32
    if (!m_pointer) return;
    typedef void* (__thiscall *PutFloat_t)(void*, float);
    auto fn = reinterpret_cast<PutFloat_t>(WoWOffsets::Packets::PutFloat);
    fn(m_pointer, value);
#else
    uint8_t buf[4];
    std::memcpy(buf, &value, 4);
    for (int i = 0; i < 4; ++i) m_hostBuffer.push_back(buf[i]);
#endif
}

void CDataStore::PutString(const std::string& value) {
#ifdef _WIN32
    if (!m_pointer) return;
    typedef void* (__thiscall *PutString_t)(void*, const char*);
    auto fn = reinterpret_cast<PutString_t>(WoWOffsets::Packets::PutString);
    fn(m_pointer, value.c_str());
#else
    for (char c : value) m_hostBuffer.push_back(static_cast<uint8_t>(c));
    m_hostBuffer.push_back(0);
#endif
}

void CDataStore::PutBytes(const uint8_t* buffer, uint32_t size) {
#ifdef _WIN32
    if (!m_pointer) return;
    typedef void* (__thiscall *PutBytes_t)(void*, const void*, uint32_t);
    auto fn = reinterpret_cast<PutBytes_t>(WoWOffsets::Packets::PutBytes);
    fn(m_pointer, buffer, size);
#else
    for (uint32_t i = 0; i < size; ++i) m_hostBuffer.push_back(buffer[i]);
#endif
}

void CDataStore::Prepare() {
#ifdef _WIN32
    if (!m_pointer) return;
    typedef void (__thiscall *Finalize_t)(void*);
    auto fn = reinterpret_cast<Finalize_t>(WoWOffsets::Packets::Finalize);
    fn(m_pointer);
#else
    m_hostBytesRead = 0;
#endif
}

void CDataStore::Destroy() {
#ifdef _WIN32
    if (!m_pointer) return;
    typedef void (__thiscall *Destroy_t)(void*);
    auto fn = reinterpret_cast<Destroy_t>(WoWOffsets::Packets::Destroy);
    fn(m_pointer);
#endif
}

} // namespace WoWMemory
