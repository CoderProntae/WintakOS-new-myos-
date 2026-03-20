#ifndef WINTAKOS_RTL8139_H
#define WINTAKOS_RTL8139_H

#include "../include/types.h"
#include "../net/netdef.h"

bool     rtl8139_init(void);
void     rtl8139_send(const void* data, uint32_t len);
bool     rtl8139_available(void);
mac_addr_t rtl8139_get_mac(void);

/* Paket alma callback */
typedef void (*rtl8139_recv_callback_t)(const void* data, uint32_t len);
void rtl8139_set_recv_callback(rtl8139_recv_callback_t cb);

#endif
