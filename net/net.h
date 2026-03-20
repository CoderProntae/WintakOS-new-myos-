#ifndef WINTAKOS_NET_H
#define WINTAKOS_NET_H

#include "../include/types.h"
#include "netdef.h"

void        net_init(void);
net_status_t* net_get_status(void);
void        net_send_arp_request(ip_addr_t target_ip);
void        net_send_ping(ip_addr_t target_ip);
void        net_process(void);

/* IP adresi olustur */
static inline ip_addr_t ip_make(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
    ip_addr_t ip;
    ip.octets[0] = a; ip.octets[1] = b;
    ip.octets[2] = c; ip.octets[3] = d;
    return ip;
}

/* MAC karsilastirma */
static inline bool mac_eq(mac_addr_t a, mac_addr_t b) {
    for (int i = 0; i < ETH_ALEN; i++) if (a.addr[i] != b.addr[i]) return false;
    return true;
}

#endif
