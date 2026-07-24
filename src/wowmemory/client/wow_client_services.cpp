#include "wow_client_services.h"
#include "wowmemory/offsets.h"
#include "wowmemory/memory_utils.h"

namespace WoWMemory {

void WoWClientServices::SendGamePacket(CDataStore& pData) {
    pData.Prepare();
#ifdef _WIN32
    typedef void (__cdecl *SendPacket_t)(void*);
    auto fn = reinterpret_cast<SendPacket_t>(WoWOffsets::ClientServices::SendPacket);
    fn(pData.GetPointer());
#endif
}

void WoWClientServices::SendPacket(CDataStore& pData) {
    pData.Prepare();
#ifdef _WIN32
    void* current = GetCurrent();
    if (current) {
        typedef void (__thiscall *SendPacket2_t)(void*, void*);
        auto fn = reinterpret_cast<SendPacket2_t>(WoWOffsets::ClientServices::SendPacket2);
        fn(current, pData.GetPointer());
    }
#endif
}

void* WoWClientServices::GetCurrent() {
#ifdef _WIN32
    typedef void* (__cdecl *GetCurrent_t)();
    auto fn = reinterpret_cast<GetCurrent_t>(WoWOffsets::ClientServices::GetCurrent);
    return fn();
#else
    return nullptr;
#endif
}

void WoWClientServices::SetMessageHandler(uint32_t msgId, PacketHandler_t handler, void* param) {
    SetMessageHandler(msgId, reinterpret_cast<void*>(handler), param);
}

void WoWClientServices::SetMessageHandler(uint32_t msgId, void* handler, void* param) {
    (void)msgId;
    (void)handler;
    (void)param;
#ifdef _WIN32
    typedef void (__cdecl *SetMessageHandler_t)(uint32_t, void*, void*);
    auto fn = reinterpret_cast<SetMessageHandler_t>(WoWOffsets::ClientServices::SetMessageHandler);
    fn(msgId, handler, param);
#endif
}

} // namespace WoWMemory
