#include "net.h"
#include "../drivers/nic.h"
#include "../cpu/pit.h"
#include "../lib/string.h"

static net_status_t status;
static mac_addr_t broadcast_mac = {{0xFF,0xFF,0xFF,0xFF,0xFF,0xFF}};
static mac_addr_t zero_mac = {{0,0,0,0,0,0}};
static uint16_t ip_id_counter = 1;
static uint32_t ping_send_tick = 0;

/* ARP tablosu */
#define ARP_TABLE_SIZE 16
static struct { ip_addr_t ip; mac_addr_t mac; bool valid; } arp_table[ARP_TABLE_SIZE];

static uint16_t ip_checksum(const void* data, uint32_t len)
{
    const uint16_t* p = (const uint16_t*)data;
    uint32_t sum = 0;
    while (len > 1) { sum += *p++; len -= 2; }
    if (len) sum += *(const uint8_t*)p;
    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    return (uint16_t)~sum;
}

static mac_addr_t* arp_lookup(ip_addr_t ip)
{
    for (int i = 0; i < ARP_TABLE_SIZE; i++)
        if (arp_table[i].valid && arp_table[i].ip.val == ip.val)
            return &arp_table[i].mac;
    return NULL;
}

static void arp_add(ip_addr_t ip, mac_addr_t mac_val)
{
    for (int i = 0; i < ARP_TABLE_SIZE; i++) {
        if (arp_table[i].valid && arp_table[i].ip.val == ip.val) {
            arp_table[i].mac = mac_val; return;
        }
    }
    for (int i = 0; i < ARP_TABLE_SIZE; i++) {
        if (!arp_table[i].valid) {
            arp_table[i].ip = ip; arp_table[i].mac = mac_val;
            arp_table[i].valid = true; return;
        }
    }
    arp_table[0].ip = ip; arp_table[0].mac = mac_val;
}

static void send_eth(mac_addr_t dst, uint16_t ethertype, const void* payload, uint32_t len)
{
    uint8_t frame[ETH_FRAME_MAX];
    eth_header_t* eth = (eth_header_t*)frame;
    memcpy(&eth->dst, &dst, ETH_ALEN);
    memcpy(&eth->src, &status.mac, ETH_ALEN);
    eth->ethertype = htons(ethertype);
    if (len > ETH_DATA_MAX) len = ETH_DATA_MAX;
    memcpy(frame + sizeof(eth_header_t), payload, len);
    nic_send(frame, sizeof(eth_header_t) + len);
    status.packets_sent++;
}

static void handle_arp(const uint8_t* data, uint32_t len)
{
    if (len < sizeof(arp_packet_t)) return;
    const arp_packet_t* arp = (const arp_packet_t*)data;
    uint16_t opcode = ntohs(arp->opcode);

    arp_add(arp->sender_ip, arp->sender_mac);

    if (opcode == ARP_REQUEST && arp->target_ip.val == status.ip.val) {
        arp_packet_t reply;
        reply.hw_type = htons(1); reply.proto_type = htons(ETH_TYPE_IP);
        reply.hw_size = 6; reply.proto_size = 4; reply.opcode = htons(ARP_REPLY);
        memcpy(&reply.sender_mac, &status.mac, ETH_ALEN);
        reply.sender_ip = status.ip;
        memcpy(&reply.target_mac, &arp->sender_mac, ETH_ALEN);
        reply.target_ip = arp->sender_ip;
        send_eth(arp->sender_mac, ETH_TYPE_ARP, &reply, sizeof(reply));
    }
}

static void handle_icmp(const ip_header_t* ip_hdr, const uint8_t* data, uint32_t len)
{
    if (len < sizeof(icmp_header_t)) return;
    const icmp_header_t* icmp = (const icmp_header_t*)data;

    if (icmp->type == ICMP_ECHO_REQUEST) {
        uint8_t reply_buf[ETH_DATA_MAX];
        ip_header_t* rip = (ip_header_t*)reply_buf;
        rip->ver_ihl = 0x45; rip->tos = 0;
        rip->total_len = htons((uint16_t)(sizeof(ip_header_t) + len));
        rip->id = htons(ip_id_counter++); rip->frag_off = 0;
        rip->ttl = 64; rip->protocol = IP_PROTO_ICMP; rip->checksum = 0;
        rip->src = status.ip; rip->dst = ip_hdr->src;
        rip->checksum = ip_checksum(rip, sizeof(ip_header_t));

        memcpy(reply_buf + sizeof(ip_header_t), data, len);
        icmp_header_t* ricmp = (icmp_header_t*)(reply_buf + sizeof(ip_header_t));
        ricmp->type = ICMP_ECHO_REPLY; ricmp->checksum = 0;
        ricmp->checksum = ip_checksum(ricmp, len);

        mac_addr_t* dst_mac = arp_lookup(ip_hdr->src);
        send_eth(dst_mac ? *dst_mac : broadcast_mac, ETH_TYPE_IP, reply_buf, sizeof(ip_header_t) + len);
    }
    else if (icmp->type == ICMP_ECHO_REPLY && ping_send_tick > 0) {
        status.ping_ms = (pit_get_ticks() - ping_send_tick) * 1000 / PIT_FREQUENCY;
        ping_send_tick = 0;
    }
}

static void handle_ip(const uint8_t* data, uint32_t len)
{
    if (len < sizeof(ip_header_t)) return;
    const ip_header_t* ip = (const ip_header_t*)data;
    if ((ip->ver_ihl >> 4) != 4) return;
    uint32_t ihl = (ip->ver_ihl & 0x0F) * 4;
    if (ihl < 20 || len < ihl) return;
    if (ip->dst.val != status.ip.val && ip->dst.val != 0xFFFFFFFF) return;

    const uint8_t* payload = data + ihl;
    uint32_t plen = ntohs(ip->total_len) - ihl;

    if (ip->protocol == IP_PROTO_ICMP) handle_icmp(ip, payload, plen);
}

static void recv_handler(const void* data, uint32_t len)
{
    if (len < sizeof(eth_header_t)) return;
    const eth_header_t* eth = (const eth_header_t*)data;
    status.packets_recv++;

    uint16_t et = ntohs(eth->ethertype);
    const uint8_t* payload = (const uint8_t*)data + sizeof(eth_header_t);
    uint32_t plen = len - sizeof(eth_header_t);

    if (et == ETH_TYPE_ARP) handle_arp(payload, plen);
    else if (et == ETH_TYPE_IP) handle_ip(payload, plen);
}

void net_init(void)
{
    memset(&status, 0, sizeof(status));
    memset(arp_table, 0, sizeof(arp_table));

    status.ip = ip_make(10, 0, 2, 15);
    status.gateway = ip_make(10, 0, 2, 2);
    status.subnet = ip_make(255, 255, 255, 0);

    nic_init();

    if (nic_available()) {
        status.mac = nic_get_mac();
        status.link_up = true;
        status.nic_found = true;
        nic_set_recv_callback(recv_handler);
    }
}

void net_send_arp_request(ip_addr_t target_ip)
{
    arp_packet_t arp;
    arp.hw_type = htons(1); arp.proto_type = htons(ETH_TYPE_IP);
    arp.hw_size = 6; arp.proto_size = 4; arp.opcode = htons(ARP_REQUEST);
    memcpy(&arp.sender_mac, &status.mac, ETH_ALEN);
    arp.sender_ip = status.ip;
    memcpy(&arp.target_mac, &zero_mac, ETH_ALEN);
    arp.target_ip = target_ip;
    send_eth(broadcast_mac, ETH_TYPE_ARP, &arp, sizeof(arp));
}

void net_send_ping(ip_addr_t target_ip)
{
    /* Once ARP sor */
    if (!arp_lookup(target_ip)) net_send_arp_request(target_ip);

    uint8_t buf[sizeof(ip_header_t) + sizeof(icmp_header_t) + 32];
    memset(buf, 0, sizeof(buf));
    ip_header_t* ip = (ip_header_t*)buf;
    ip->ver_ihl = 0x45; ip->total_len = htons(sizeof(buf));
    ip->id = htons(ip_id_counter++); ip->ttl = 64; ip->protocol = IP_PROTO_ICMP;
    ip->src = status.ip; ip->dst = target_ip;
    ip->checksum = 0; ip->checksum = ip_checksum(ip, sizeof(ip_header_t));

    icmp_header_t* icmp = (icmp_header_t*)(buf + sizeof(ip_header_t));
    icmp->type = ICMP_ECHO_REQUEST; icmp->id = htons(0x1234); icmp->seq = htons(1);
    icmp->checksum = 0; icmp->checksum = ip_checksum(icmp, sizeof(icmp_header_t) + 32);

    ping_send_tick = pit_get_ticks();
    mac_addr_t* dm = arp_lookup(target_ip);
    send_eth(dm ? *dm : broadcast_mac, ETH_TYPE_IP, buf, sizeof(buf));
}

net_status_t* net_get_status(void) { return &status; }
void net_process(void) { }
