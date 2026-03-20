#include "pcnet.h"
#include "nic.h"
#include "../cpu/pci.h"
#include "../cpu/ports.h"
#include "../cpu/isr.h"
#include "../cpu/pic.h"
#include "../lib/string.h"

#define PCNET_RDP    0x10
#define PCNET_RAP    0x14
#define PCNET_RESET  0x18
#define PCNET_BDP    0x1C

#define DESC_COUNT   8
#define BUF_SIZE     1548

typedef struct {
    uint32_t addr;
    uint16_t length;
    int16_t  status;
    uint32_t misc;
    uint32_t reserved;
} __attribute__((packed)) pcnet_desc_t;

typedef struct {
    uint16_t mode;
    uint8_t  rlen;
    uint8_t  tlen;
    uint8_t  mac[6];
    uint16_t reserved;
    uint8_t  ladr[8];
    uint32_t rdra;
    uint32_t tdra;
} __attribute__((packed)) pcnet_init_block_t;

static bool found = false;
static uint16_t io_base = 0;
static mac_addr_t mac;
static pci_device_t pci_dev;

static pcnet_init_block_t init_block __attribute__((aligned(16)));
static pcnet_desc_t rx_descs[DESC_COUNT] __attribute__((aligned(16)));
static pcnet_desc_t tx_descs[DESC_COUNT] __attribute__((aligned(16)));
static uint8_t rx_bufs[DESC_COUNT][BUF_SIZE] __attribute__((aligned(16)));
static uint8_t tx_bufs[DESC_COUNT][BUF_SIZE] __attribute__((aligned(16)));
static uint32_t rx_cur = 0, tx_cur = 0;

static void pcnet_write_csr(uint32_t csr, uint32_t val) {
    outl(io_base + PCNET_RAP, csr);
    outl(io_base + PCNET_RDP, val);
}
static uint32_t pcnet_read_csr(uint32_t csr) {
    outl(io_base + PCNET_RAP, csr);
    return inl(io_base + PCNET_RDP);
}
static void pcnet_write_bcr(uint32_t bcr, uint32_t val) {
    outl(io_base + PCNET_RAP, bcr);
    outl(io_base + PCNET_BDP, val);
}

static void pcnet_handler(registers_t* regs)
{
    (void)regs;
    uint32_t csr0 = pcnet_read_csr(0);
    pcnet_write_csr(0, csr0); /* Temizle */

    if (csr0 & (1 << 10)) { /* RINT */
        while (!(rx_descs[rx_cur].status & (1 << 15))) {
            if (rx_descs[rx_cur].status & (1 << 8)) { /* STP */
                uint16_t len = (uint16_t)(rx_descs[rx_cur].misc & 0xFFF);
                if (len > 0 && len <= BUF_SIZE)
                    nic_dispatch_recv(rx_bufs[rx_cur], len - 4);
            }
            rx_descs[rx_cur].status = (int16_t)(1 << 15);
            rx_descs[rx_cur].misc = 0;
            rx_cur = (rx_cur + 1) % DESC_COUNT;
        }
    }
}

bool pcnet_init(void)
{
    if (!pci_find_by_id(0x1022, 0x2000, &pci_dev)) { found = false; return false; }

    io_base = (uint16_t)(pci_dev.bar0 & 0xFFFC);
    if (io_base == 0) { found = false; return false; }
    pci_enable_bus_master(&pci_dev);

    /* Reset */
    inl(io_base + PCNET_RESET);
    for (volatile int i = 0; i < 50000; i++);

    /* 32-bit mod */
    outl(io_base + PCNET_RDP, 0);
    pcnet_write_bcr(20, 0x0102); /* SSIZE32 */

    /* MAC oku */
    for (int i = 0; i < 6; i++)
        mac.addr[i] = inb(io_base + i);

    /* Descriptor'lar */
    memset(rx_descs, 0, sizeof(rx_descs));
    memset(tx_descs, 0, sizeof(tx_descs));
    for (uint32_t i = 0; i < DESC_COUNT; i++) {
        rx_descs[i].addr = (uint32_t)rx_bufs[i];
        rx_descs[i].length = (uint16_t)(-(int16_t)BUF_SIZE);
        rx_descs[i].status = (int16_t)(1 << 15); /* OWN */
        tx_descs[i].addr = (uint32_t)tx_bufs[i];
        tx_descs[i].status = 0;
    }

    /* Init block */
    memset(&init_block, 0, sizeof(init_block));
    init_block.mode = 0;
    init_block.rlen = 3 << 4; /* log2(8) = 3 */
    init_block.tlen = 3 << 4;
    for (int i = 0; i < 6; i++) init_block.mac[i] = mac.addr[i];
    init_block.rdra = (uint32_t)rx_descs;
    init_block.tdra = (uint32_t)tx_descs;

    pcnet_write_csr(1, (uint32_t)&init_block & 0xFFFF);
    pcnet_write_csr(2, ((uint32_t)&init_block >> 16) & 0xFFFF);

    /* INIT + START */
    pcnet_write_csr(0, 0x0041);
    int timeout = 100000;
    while (!(pcnet_read_csr(0) & (1 << 8)) && --timeout);

    pcnet_write_csr(0, 0x0042); /* START + IENA */

    /* IRQ */
    pcnet_write_csr(4, pcnet_read_csr(4) | (1 << 10)); /* RINT enable */
    if (pci_dev.irq > 0 && pci_dev.irq < 16) {
        irq_register_handler(pci_dev.irq, pcnet_handler);
        pic_clear_mask(pci_dev.irq);
    }

    rx_cur = 0; tx_cur = 0;
    found = true;
    return true;
}

void pcnet_send(const void* data, uint32_t len)
{
    if (!found || len > BUF_SIZE) return;
    while (tx_descs[tx_cur].status & (int16_t)(1 << 15));

    memcpy(tx_bufs[tx_cur], data, len);
    if (len < 60) len = 60;

    tx_descs[tx_cur].length = (uint16_t)(-(int16_t)len);
    tx_descs[tx_cur].misc = 0;
    tx_descs[tx_cur].status = (int16_t)((1 << 15) | (1 << 9) | (1 << 8)); /* OWN|STP|ENP */

    pcnet_write_csr(0, 0x0048); /* TDMD */
    tx_cur = (tx_cur + 1) % DESC_COUNT;
}

mac_addr_t pcnet_get_mac(void) { return mac; }
