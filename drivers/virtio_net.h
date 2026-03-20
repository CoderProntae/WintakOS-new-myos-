#ifndef WINTAKOS_VIRTIO_NET_H
#define WINTAKOS_VIRTIO_NET_H

#include "../include/types.h"
#include "../net/netdef.h"

bool       virtio_net_init(void);
void       virtio_net_send(const void* data, uint32_t len);
mac_addr_t virtio_net_get_mac(void);

#endif
