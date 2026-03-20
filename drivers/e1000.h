#ifndef WINTAKOS_E1000_H
#define WINTAKOS_E1000_H

#include "../include/types.h"
#include "../net/netdef.h"

bool       e1000_init(void);
void       e1000_send(const void* data, uint32_t len);
mac_addr_t e1000_get_mac(void);

#endif
