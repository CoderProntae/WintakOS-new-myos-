#ifndef WINTAKOS_PCI_H
#define WINTAKOS_PCI_H

#include "../include/types.h"

typedef struct {
    uint8_t  bus;
    uint8_t  slot;
    uint8_t  func;
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t  class_code;
    uint8_t  subclass;
    uint32_t bar0;
    uint32_t bar1;
    uint8_t  irq;
    bool     found;
} pci_device_t;

uint32_t pci_config_read(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
void     pci_config_write(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t val);
bool     pci_find_device(uint8_t class_code, uint8_t subclass, pci_device_t* out);
bool     pci_find_by_id(uint16_t vendor, uint16_t device, pci_device_t* out);
void     pci_enable_bus_master(pci_device_t* dev);

#endif
