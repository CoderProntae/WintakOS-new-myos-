#include "e1000.h"
#include "nic.h"
#include "../cpu/pci.h"
#include "../cpu/ports.h"
#include "../cpu/isr.h"
#include "../cpu/pic.h"
#include "../lib/string.h"

/* E1000 Registers */
#define E1000_CTRL     0x0000
#define E1000_STATUS   0x0008
#define E1000_EERD     0x0014
#define E1000_ICR      0x00C0
#define E1000_IMS      0x00D0
#define E1000_IMC      0x00D8
#define E1000_RCTL     0x0100
#define E1000_RDBAL    0x2800
#define E1000_RDLEN    0x2808
#define E1000_RDH      0x2810
#define E1000_RDT      0x2818
#define E1000_TCTL     0x0400
#define E1000_TDBAL    0x3800
#define E1000_TDLEN    0x3808
#define E1000_TDH      0x3810
#define E1000_TDT      0x3818
#define E1000_RAL0     0x5400
#define E1000_RAH0     0x5404
#define E1000_MTA      0x5200

#define E1000_CTRL_RST   (1 << 26)
#define E1000_CTRL_SLU   (1 << 6)
#define E1000_CTRL_ASDE  (1 << 5)
#define E1000_RCTL_EN    (1 << 1)
#define E1000_RCTL_BAM   (1 << 15)
#define E1000_RCTL_BSIZE (0 << 16)
#define E1000_RCTL_SECRC (1 << 26)
#define E1000_TCTL_EN    (1 << 1)
#define E1000_TCTL_PSP   (1 << 3)

#define RX_DESC_COUNT  32
#define TX_DESC_COUNT  8

typedef struct {
    uint64_t addr;
    uint16_t length;
    uint16_t checksum;
    uint8_t  status;
    uint8_t  errors;
    uint16_t special;
} __attribute__((packed)) e1000_rx_desc_t;

typedef struct {
    uint64_t addr;
    uint16_t length;
    uint8_t  cso;
    uint8_t  cmd;
    uint8_t  status;
    uint8_t  css;
    uint16_t special;
} __attribute__((packed)) e1000_tx_desc_t;

static bool found = false;
static volatile uint32_t* mmio_base = NULL;
static mac_addr_t mac;
static pci_device_t pci_dev;

static e1000_rx_desc_t rx_descs[RX_DESC_COUNT] __attribute__((aligned(128)));
static e1000_tx_desc_t tx_descs[TX_DESC_COUNT] __attribute__((aligned(128)));
static uint8_t rx_buffers[RX_DESC_COUNT][2048] __attribute__((aligned(16)));
static uint8_t tx_buffers[TX_DESC_COUNT][2048] __attribute__((aligned(16)));
static uint32_t rx_cur = 0;
static uint32_t tx_cur = 0;

static void e1000_write(uint32_t reg, uint32_t val) { mmio_base[reg / 4] = val; }
static uint32_t e1000_read(uint32_t reg) { return mmio_base[reg / 4]; }

static void e1000_handler(registers_t* regs)
{
    (void)regs;
    uint32_t icr = e1000_read(E1000_ICR);

    if (icr & 0x80) { /* RX */
        while (rx_descs[rx_cur].status & 0x01) {
            uint16_t len = rx_descs[rx_cur].length;
            if (len > 0 && len <= 2048) {
                nic_dispatch_recv(rx_buffers[rx_cur], len);
            }
            rx_descs[rx_cur].status = 0;
            uint32_t old = rx_cur;
            rx_cur = (rx_cur + 1) % RX_DESC_COUNT;
            e1000_write(E1000_RDT, old);
        }
    }
}

static bool e1000_detect(void)
{
    /* Bilinen Intel E1000 device ID'leri */
    uint16_t ids[] = { 0x100E, 0x100F, 0x10D3, 0x10EA, 0x153A, 0x1503 };
    for (uint32_t i = 0; i < 6; i++) {
        if (pci_find_by_id(0x8086, ids[i], &pci_dev)) return true;
    }
    /* Genel multimedia/network class taramasi */
    if (pci_find_device(0x02, 0x00, &pci_dev) && pci_dev.vendor_id == 0x8086)
        return true;
    return false;
}

bool e1000_init(void)
{
    if (!e1000_detect()) { found = false; return false; }

    uint32_t bar0 = pci_dev.bar0 & ~0x0F;
    if (bar0 == 0) { found = false; return false; }

    mmio_base = (volatile uint32_t*)(uintptr_t)bar0;
    pci_enable_bus_master(&pci_dev);

    /* Reset */
    e1000_write(E1000_CTRL, e1000_read(E1000_CTRL) | E1000_CTRL_RST);
    for (volatile int i = 0; i < 100000; i++);

    /* Link up */
    e1000_write(E1000_CTRL, E1000_CTRL_SLU | E1000_CTRL_ASDE);

    /* Interrupt maskeleri temizle, sonra ayarla */
    e1000_write(E1000_IMC, 0xFFFFFFFF);
    e1000_read(E1000_ICR);

    /* MAC adresi oku */
    uint32_t ral = e1000_read(E1000_RAL0);
    uint32_t rah = e1000_read(E1000_RAH0);
    mac.addr[0] = (uint8_t)(ral);
    mac.addr[1] = (uint8_t)(ral >> 8);
    mac.addr[2] = (uint8_t)(ral >> 16);
    mac.addr[3] = (uint8_t)(ral >> 24);
    mac.addr[4] = (uint8_t)(rah);
    mac.addr[5] = (uint8_t)(rah >> 8);

    /* Sifir MAC ise EEPROM'dan oku */
    if (mac.addr[0] == 0 && mac.addr[1] == 0 && mac.addr[2] == 0) {
        for (int i = 0; i < 3; i++) {
            e1000_write(E1000_EERD, (uint32_t)(1 | (i << 8)));
            uint32_t val;
            int timeout = 10000;
            do { val = e1000_read(E1000_EERD); } while (!(val & 0x10) && --timeout);
            val >>= 16;
            mac.addr[i * 2]     = (uint8_t)(val & 0xFF);
            mac.addr[i * 2 + 1] = (uint8_t)(val >> 8);
        }
    }

    /* Multicast tablosunu temizle */
    for (int i = 0; i < 128; i++) e1000_write(E1000_MTA + i * 4, 0);

    /* RX descriptors */
    memset(rx_descs, 0, sizeof(rx_descs));
    for (uint32_t i = 0; i < RX_DESC_COUNT; i++) {
        rx_descs[i].addr = (uint64_t)(uint32_t)rx_buffers[i];
    }
    e1000_write(E1000_RDBAL, (uint32_t)rx_descs);
    e1000_write(E1000_RDBAL + 4, 0);
    e1000_write(E1000_RDLEN, RX_DESC_COUNT * sizeof(e1000_rx_desc_t));
    e1000_write(E1000_RDH, 0);
    e1000_write(E1000_RDT, RX_DESC_COUNT - 1);
    rx_cur = 0;

    e1000_write(E1000_RCTL, E1000_RCTL_EN | E1000_RCTL_BAM | E1000_RCTL_BSIZE | E1000_RCTL_SECRC);

    /* TX descriptors */
    memset(tx_descs, 0, sizeof(tx_descs));
    for (uint32_t i = 0; i < TX_DESC_COUNT; i++) {
        tx_descs[i].addr = (uint64_t)(uint32_t)tx_buffers[i];
        tx_descs[i].status = 0x01; /* DD — done */
    }
    e1000_write(E1000_TDBAL, (uint32_t)tx_descs);
    e1000_write(E1000_TDBAL + 4, 0);
    e1000_write(E1000_TDLEN, TX_DESC_COUNT * sizeof(e1000_tx_desc_t));
    e1000_write(E1000_TDH, 0);
    e1000_write(E1000_TDT, 0);
    tx_cur = 0;

    e1000_write(E1000_TCTL, E1000_TCTL_EN | E1000_TCTL_PSP | (15 << 4) | (64 << 12));

    /* Interrupt ayarla */
    e1000_write(E1000_IMS, 0x84); /* RX + Link Status Change */

    if (pci_dev.irq > 0 && pci_dev.irq < 16) {
        irq_register_handler(pci_dev.irq, e1000_handler);
        pic_clear_mask(pci_dev.irq);
    }

    found = true;
    return true;
}

void e1000_send(const void* data, uint32_t len)
{
    if (!found || len > 1518) return;

    while (!(tx_descs[tx_cur].status & 0x01));

    memcpy(tx_buffers[tx_cur], data, len);
    tx_descs[tx_cur].length = (uint16_t)len;
    tx_descs[tx_cur].cmd = (1 << 0) | (1 << 1) | (1 << 3); /* EOP + IFCS + RS */
    tx_descs[tx_cur].status = 0;

    uint32_t old = tx_cur;
    tx_cur = (tx_cur + 1) % TX_DESC_COUNT;
    e1000_write(E1000_TDT, tx_cur);
    (void)old;
}

mac_addr_t e1000_get_mac(void) { return mac; }
