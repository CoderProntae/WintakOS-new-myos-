#ifndef WINTAKOS_PCNET_H
#define WINTAKOS_PCNET_H

#include "../include/types.h"
#include "../net/netdef.h"

bool       pcnet_init(void);
void       pcnet_send(const void* data, uint32_t len);
mac_addr_t pcnet_get_mac(void);

#endif
