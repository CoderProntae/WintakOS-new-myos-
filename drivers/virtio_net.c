#include "virtio_net.h"
#include "../cpu/pci.h"

static mac_addr_t mac = {{0,0,0,0,0,0}};

bool virtio_net_init(void)
{
    pci_device_t dev;
    if (!pci_find_by_id(0x1AF4, 0x1000, &dev)) return false;
    /* VirtIO tam implementasyon ileride eklenecek */
    /* Simdilik sadece detect et, kullanma */
    (void)dev;
    return false;
}

void virtio_net_send(const void* data, uint32_t len) { (void)data; (void)len; }
mac_addr_t virtio_net_get_mac(void) { return mac; }
