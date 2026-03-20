#ifndef WINTAKOS_NIC_H
#define WINTAKOS_NIC_H

#include "../include/types.h"
#include "../net/netdef.h"

typedef void (*nic_recv_cb_t)(const void* data, uint32_t len);

typedef struct {
    const char* name;
    bool     (*init)(void);
    void     (*send)(const void* data, uint32_t len);
    mac_addr_t (*get_mac)(void);
    bool     available;
} nic_driver_t;

void       nic_init(void);
bool       nic_available(void);
void       nic_send(const void* data, uint32_t len);
mac_addr_t nic_get_mac(void);
const char* nic_get_name(void);
void       nic_set_recv_callback(nic_recv_cb_t cb);
void       nic_dispatch_recv(const void* data, uint32_t len);

#endif
