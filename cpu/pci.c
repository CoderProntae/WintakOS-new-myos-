#include "pci.h"
#include "ports.h"

#define PCI_CONFIG_ADDR  0xCF8
#define PCI_CONFIG_DATA  0xCFC

uint32_t pci_config_read(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset)
{
    uint32_t addr = (uint32_t)(1 << 31) |
                    ((uint32_t)bus << 16) |
                    ((uint32_t)slot << 11) |
                    ((uint32_t)func << 8) |
                    ((uint32_t)offset & 0xFC);
    outl(PCI_CONFIG_ADDR, addr);
    return inl(PCI_CONFIG_DATA);
}

void pci_config_write(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t val)
{
    uint32_t addr = (uint32_t)(1 << 31) |
                    ((uint32_t)bus << 16) |
                    ((uint32_t)slot << 11) |
                    ((uint32_t)func << 8) |
                    ((uint32_t)offset & 0xFC);
    outl(PCI_CONFIG_ADDR, addr);
    outl(PCI_CONFIG_DATA, val);
}

static void pci_read_device(uint8_t bus, uint8_t slot, uint8_t func, pci_device_t* dev)
{
    uint32_t reg0 = pci_config_read(bus, slot, func, 0x00);
    dev->vendor_id = (uint16_t)(reg0 & 0xFFFF);
    dev->device_id = (uint16_t)(reg0 >> 16);
    dev->bus = bus;
    dev->slot = slot;
    dev->func = func;

    uint32_t reg2 = pci_config_read(bus, slot, func, 0x08);
    dev->class_code = (uint8_t)(reg2 >> 24);
    dev->subclass = (uint8_t)((reg2 >> 16) & 0xFF);

    dev->bar0 = pci_config_read(bus, slot, func, 0x10);
    dev->bar1 = pci_config_read(bus, slot, func, 0x14);

    uint32_t reg3c = pci_config_read(bus, slot, func, 0x3C);
    dev->irq = (uint8_t)(reg3c & 0xFF);
    dev->found = true;
}

bool pci_find_device(uint8_t class_code, uint8_t subclass, pci_device_t* out)
{
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t slot = 0; slot < 32; slot++) {
            for (uint8_t func = 0; func < 8; func++) {
                uint32_t reg0 = pci_config_read((uint8_t)bus, slot, func, 0x00);
                uint16_t vendor = (uint16_t)(reg0 & 0xFFFF);
                if (vendor == 0xFFFF) continue;

                uint32_t reg2 = pci_config_read((uint8_t)bus, slot, func, 0x08);
                uint8_t cc = (uint8_t)(reg2 >> 24);
                uint8_t sc = (uint8_t)((reg2 >> 16) & 0xFF);

                if (cc == class_code && sc == subclass) {
                    pci_read_device((uint8_t)bus, slot, func, out);
                    return true;
                }
            }
        }
    }
    out->found = false;
    return false;
}

bool pci_find_by_id(uint16_t vendor, uint16_t device, pci_device_t* out)
{
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t slot = 0; slot < 32; slot++) {
            for (uint8_t func = 0; func < 8; func++) {
                uint32_t reg0 = pci_config_read((uint8_t)bus, slot, func, 0x00);
                uint16_t v = (uint16_t)(reg0 & 0xFFFF);
                uint16_t d = (uint16_t)(reg0 >> 16);
                if (v == vendor && d == device) {
                    pci_read_device((uint8_t)bus, slot, func, out);
                    return true;
                }
            }
        }
    }
    out->found = false;
    return false;
}

void pci_enable_bus_master(pci_device_t* dev)
{
    uint32_t cmd = pci_config_read(dev->bus, dev->slot, dev->func, 0x04);
    cmd |= (1 << 2); /* Bus Master Enable */
    cmd |= (1 << 0); /* I/O Space Enable */
    pci_config_write(dev->bus, dev->slot, dev->func, 0x04, cmd);
}
