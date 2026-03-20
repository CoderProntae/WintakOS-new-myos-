#include "rtl8139.h"
#include "../cpu/pci.h"
#include "../cpu/ports.h"
#include "../cpu/isr.h"
#include "../cpu/pic.h"
#include "../lib/string.h"

/* RTL8139 Registers */
#define RTL_IDR0       0x00
#define RTL_MAR0       0x08
#define RTL_TXSTATUS0  0x10
#define RTL_TXADDR0    0x20
#define RTL_RXBUF      0x30
#define RTL_CMD        0x37
#define RTL_CAPR       0x38
#define RTL_CBR        0x3A
#define RTL_IMR        0x3C
#define RTL_ISR        0x3E
#define RTL_TXCONFIG   0x40
#define RTL_RXCONFIG   0x44
#define RTL_CONFIG1    0x52

#define RTL_CMD_RESET   0x10
#define RTL_CMD_RX_EN   0x08
#define RTL_CMD_TX_EN   0x04

#define RX_BUF_SIZE     (8192 + 16 + 1500)
#define TX_BUF_SIZE     1536

static bool     nic_found = false;
static uint16_t io_base = 0;
static mac_addr_t nic_mac;

static uint8_t rx_buffer[RX_BUF_SIZE + 16] __attribute__((aligned(16)));
static uint8_t tx_buffers[4][TX_BUF_SIZE] __attribute__((aligned(16)));
static uint8_t tx_cur = 0;
static uint16_t rx_offset = 0;

static rtl8139_recv_callback_t recv_cb = NULL;
static pci_device_t nic_pci;

static void rtl8139_handler(registers_t* regs)
{
    (void)regs;

    uint16_t status = inw(io_base + RTL_ISR);
    outw(io_base + RTL_ISR, status); /* Temizle */

    if (status & 0x01) { /* ROK — paket alindi */
        while (!(inb(io_base + RTL_CMD) & 0x01)) { /* Buffer bos degil */
            uint8_t* buf = rx_buffer + rx_offset;

            uint16_t pkt_status = *(uint16_t*)buf;
            uint16_t pkt_len    = *(uint16_t*)(buf + 2);

            if (pkt_len == 0 || pkt_len > ETH_FRAME_MAX || !(pkt_status & 0x01)) {
                break;
            }

            uint8_t* pkt_data = buf + 4;

            if (recv_cb) {
                recv_cb(pkt_data, pkt_len - 4); /* CRC cikar */
            }

            rx_offset = (rx_offset + pkt_len + 4 + 3) & ~3;
            rx_offset %= RX_BUF_SIZE;
            outw(io_base + RTL_CAPR, rx_offset - 0x10);
        }
    }
}

bool rtl8139_init(void)
{
    /* RTL8139: Vendor=0x10EC, Device=0x8139 */
    if (!pci_find_by_id(0x10EC, 0x8139, &nic_pci)) {
        nic_found = false;
        return false;
    }

    io_base = (uint16_t)(nic_pci.bar0 & 0xFFFC);
    if (io_base == 0) { nic_found = false; return false; }

    pci_enable_bus_master(&nic_pci);

    /* Power on */
    outb(io_base + RTL_CONFIG1, 0x00);

    /* Software Reset */
    outb(io_base + RTL_CMD, RTL_CMD_RESET);
    while (inb(io_base + RTL_CMD) & RTL_CMD_RESET);

    /* MAC adresi oku */
    for (int i = 0; i < 6; i++) {
        nic_mac.addr[i] = inb(io_base + RTL_IDR0 + i);
    }

    /* RX buffer ayarla */
    memset(rx_buffer, 0, sizeof(rx_buffer));
    outl(io_base + RTL_RXBUF, (uint32_t)rx_buffer);
    rx_offset = 0;

    /* IMR: ROK + TOK */
    outw(io_base + RTL_IMR, 0x0005);

    /* RX config: Accept Broadcast + Multicast + Physical + WRAP */
    outl(io_base + RTL_RXCONFIG, 0x0000000F);

    /* TX config */
    outl(io_base + RTL_TXCONFIG, 0x03000700);

    /* RX ve TX etkinlestir */
    outb(io_base + RTL_CMD, RTL_CMD_RX_EN | RTL_CMD_TX_EN);

    /* IRQ handler kaydet */
    if (nic_pci.irq > 0 && nic_pci.irq < 16) {
        irq_register_handler(nic_pci.irq, rtl8139_handler);
        pic_clear_mask(nic_pci.irq);
    }

    nic_found = true;
    return true;
}

void rtl8139_send(const void* data, uint32_t len)
{
    if (!nic_found || len > TX_BUF_SIZE) return;

    memcpy(tx_buffers[tx_cur], data, len);
    if (len < 60) len = 60; /* Minimum ethernet frame */

    outl(io_base + RTL_TXADDR0 + tx_cur * 4, (uint32_t)tx_buffers[tx_cur]);
    outl(io_base + RTL_TXSTATUS0 + tx_cur * 4, len);

    tx_cur = (tx_cur + 1) % 4;
}

bool rtl8139_available(void) { return nic_found; }

mac_addr_t rtl8139_get_mac(void) { return nic_mac; }

void rtl8139_set_recv_callback(rtl8139_recv_callback_t cb) { recv_cb = cb; }
