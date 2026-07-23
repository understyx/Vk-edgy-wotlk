#ifndef WOWMEMORY_CLIENT_WOW_CLIENT_SERVICES_H
#define WOWMEMORY_CLIENT_WOW_CLIENT_SERVICES_H

#include "c_data_store.h"

#ifndef _WIN32
#ifndef __cdecl
#define __cdecl
#endif
#endif

namespace WoWMemory {

typedef int (__cdecl *PacketHandler_t)(void* param, uint32_t msgId, uint32_t time, void* pData);

class WoWClientServices {
public:
    static void SendGamePacket(CDataStore& pData);
    static void SendPacket(CDataStore& pData);
    static void* GetCurrent();
    static void SetMessageHandler(uint32_t msgId, PacketHandler_t handler, void* param);
    static void SetMessageHandler(uint32_t msgId, void* handler, void* param);
};

} // namespace WoWMemory

#endif // WOWMEMORY_CLIENT_WOW_CLIENT_SERVICES_H
