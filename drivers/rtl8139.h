#ifndef WINTAKOS_RTL8139_H
#define WINTAKOS_RTL8139_H

#include "../include/types.h"

#define ETH_MTU         1500
#define ETH_FRAME_MAX   1518

typedef struct {
    uint8_t  mac[6];
    uint32_t ip;
    uint16_t io_base;
    uint8_t  irq;
    bool     available;
    bool     link_up;
} nic_info_t;

bool     rtl8139_init(void);
void     rtl8139_send(const void* data, uint32_t len);
nic_info_t* rtl8139_get_info(void);

/* Paket isleme callback */
typedef void (*packet_handler_t)(const void* data, uint32_t len);
void rtl8139_set_handler(packet_handler_t handler);
void rtl8139_poll(void);

#endif
