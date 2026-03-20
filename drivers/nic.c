#include "nic.h"
#include "rtl8139.h"
#include "e1000.h"
#include "pcnet.h"
#include "virtio_net.h"

static nic_driver_t* active_nic = NULL;
static nic_recv_cb_t global_recv_cb = NULL;

/* NIC dispatch — tum driverlar buraya yonlendirir */
void nic_dispatch_recv(const void* data, uint32_t len)
{
    if (global_recv_cb) global_recv_cb(data, len);
}

void nic_init(void)
{
    /* Oncelik sirasi: E1000 > VirtIO > PCnet > RTL8139 */
    static nic_driver_t drivers[] = {
        { "Intel E1000", e1000_init, e1000_send, e1000_get_mac, false },
        { "VirtIO-Net",  virtio_net_init, virtio_net_send, virtio_net_get_mac, false },
        { "AMD PCnet",   pcnet_init, pcnet_send, pcnet_get_mac, false },
        { "RTL8139",     rtl8139_init, rtl8139_send, rtl8139_get_mac, false },
    };

    for (uint32_t i = 0; i < 4; i++) {
        if (drivers[i].init()) {
            drivers[i].available = true;
            active_nic = &drivers[i];
            return;
        }
    }
}

bool nic_available(void) { return active_nic != NULL; }

void nic_send(const void* data, uint32_t len)
{
    if (active_nic) active_nic->send(data, len);
}

mac_addr_t nic_get_mac(void)
{
    if (active_nic) return active_nic->get_mac();
    mac_addr_t z = {{0,0,0,0,0,0}};
    return z;
}

const char* nic_get_name(void)
{
    if (active_nic) return active_nic->name;
    return "Yok";
}

void nic_set_recv_callback(nic_recv_cb_t cb) { global_recv_cb = cb; }
