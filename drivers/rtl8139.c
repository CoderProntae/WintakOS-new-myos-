#include "rtl8139.h"
#include "nic.h"
#include "../cpu/pci.h"
#include "../cpu/ports.h"
#include "../cpu/isr.h"
#include "../cpu/pic.h"
#include "../lib/string.h"

#define RTL_IDR0       0x00
#define RTL_TXSTATUS0  0x10
#define RTL_TXADDR0    0x20
#define RTL_RXBUF      0x30
#define RTL_CMD        0x37
#define RTL_CAPR       0x38
#define RTL_IMR        0x3C
#define RTL_ISR        0x3E
#define RTL_TXCONFIG   0x40
#define RTL_RXCONFIG   0x44
#define RTL_CONFIG1    0x52

#define RX_BUF_SIZE    (8192 + 16 + 1500)
#define TX_BUF_SIZE    1536

static bool found = false;
static uint16_t io_base = 0;
static mac_addr_t nic_mac;
static uint8_t rx_buffer[RX_BUF_SIZE + 16] __attribute__((aligned(16)));
static uint8_t tx_buffers[4][TX_BUF_SIZE] __attribute__((aligned(16)));
static uint8_t tx_cur = 0;
static uint16_t rx_offset = 0;
static pci_device_t nic_pci;

static void rtl8139_handler(registers_t* regs)
{
    (void)regs;
    uint16_t status = inw(io_base + RTL_ISR);
    outw(io_base + RTL_ISR, status);

    if (status & 0x01) {
        while (!(inb(io_base + RTL_CMD) & 0x01)) {
            uint16_t pkt_status = *(uint16_t*)(rx_buffer + rx_offset);
            uint16_t pkt_len = *(uint16_t*)(rx_buffer + rx_offset + 2);
            if (pkt_len == 0 || pkt_len > 1518 || !(pkt_status & 0x01)) break;

            nic_dispatch_recv(rx_buffer + rx_offset + 4, pkt_len - 4);

            rx_offset = (rx_offset + pkt_len + 4 + 3) & ~3;
            rx_offset %= RX_BUF_SIZE;
            outw(io_base + RTL_CAPR, rx_offset - 0x10);
        }
    }
}

bool rtl8139_init(void)
{
    if (!pci_find_by_id(0x10EC, 0x8139, &nic_pci)) { found = false; return false; }
    io_base = (uint16_t)(nic_pci.bar0 & 0xFFFC);
    if (io_base == 0) { found = false; return false; }
    pci_enable_bus_master(&nic_pci);

    outb(io_base + RTL_CONFIG1, 0x00);
    outb(io_base + RTL_CMD, 0x10);
    while (inb(io_base + RTL_CMD) & 0x10);

    for (int i = 0; i < 6; i++) nic_mac.addr[i] = inb(io_base + RTL_IDR0 + i);

    memset(rx_buffer, 0, sizeof(rx_buffer));
    outl(io_base + RTL_RXBUF, (uint32_t)rx_buffer);
    rx_offset = 0;

    outw(io_base + RTL_IMR, 0x0005);
    outl(io_base + RTL_RXCONFIG, 0x0000000F);
    outl(io_base + RTL_TXCONFIG, 0x03000700);
    outb(io_base + RTL_CMD, 0x0C);

    if (nic_pci.irq > 0 && nic_pci.irq < 16) {
        irq_register_handler(nic_pci.irq, rtl8139_handler);
        pic_clear_mask(nic_pci.irq);
    }

    found = true;
    return true;
}

void rtl8139_send(const void* data, uint32_t len)
{
    if (!found || len > TX_BUF_SIZE) return;
    memcpy(tx_buffers[tx_cur], data, len);
    if (len < 60) len = 60;
    outl(io_base + RTL_TXADDR0 + tx_cur * 4, (uint32_t)tx_buffers[tx_cur]);
    outl(io_base + RTL_TXSTATUS0 + tx_cur * 4, len);
    tx_cur = (tx_cur + 1) % 4;
}

bool rtl8139_available(void) { return found; }
mac_addr_t rtl8139_get_mac(void) { return nic_mac; }
