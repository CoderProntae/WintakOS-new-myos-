#ifndef WINTAKOS_NETDEF_H
#define WINTAKOS_NETDEF_H

#include "../include/types.h"

/* Byte sırası dönüşüm (x86 = little-endian, ağ = big-endian) */
static inline uint16_t htons(uint16_t v) { return (v >> 8) | (v << 8); }
static inline uint16_t ntohs(uint16_t v) { return htons(v); }
static inline uint32_t htonl(uint32_t v) {
    return ((v >> 24) & 0xFF) | ((v >> 8) & 0xFF00) |
           ((v << 8) & 0xFF0000) | ((v << 24) & 0xFF000000);
}
static inline uint32_t ntohl(uint32_t v) { return htonl(v); }

#define ETH_ALEN        6
#define ETH_FRAME_MAX   1518
#define ETH_DATA_MAX    1500

/* EtherType */
#define ETH_TYPE_ARP    0x0806
#define ETH_TYPE_IP     0x0800

/* IP Protocol */
#define IP_PROTO_ICMP   1
#define IP_PROTO_TCP    6
#define IP_PROTO_UDP    17

/* ICMP Types */
#define ICMP_ECHO_REPLY   0
#define ICMP_ECHO_REQUEST 8

/* MAC adresi */
typedef struct {
    uint8_t addr[ETH_ALEN];
} __attribute__((packed)) mac_addr_t;

/* IP adresi */
typedef union {
    uint32_t val;
    uint8_t  octets[4];
} ip_addr_t;

/* Ethernet başlığı */
typedef struct {
    mac_addr_t dst;
    mac_addr_t src;
    uint16_t   ethertype;
} __attribute__((packed)) eth_header_t;

/* ARP paketi */
typedef struct {
    uint16_t hw_type;
    uint16_t proto_type;
    uint8_t  hw_size;
    uint8_t  proto_size;
    uint16_t opcode;
    mac_addr_t sender_mac;
    ip_addr_t  sender_ip;
    mac_addr_t target_mac;
    ip_addr_t  target_ip;
} __attribute__((packed)) arp_packet_t;

#define ARP_REQUEST  1
#define ARP_REPLY    2

/* IP başlığı */
typedef struct {
    uint8_t  ver_ihl;
    uint8_t  tos;
    uint16_t total_len;
    uint16_t id;
    uint16_t frag_off;
    uint8_t  ttl;
    uint8_t  protocol;
    uint16_t checksum;
    ip_addr_t src;
    ip_addr_t dst;
} __attribute__((packed)) ip_header_t;

/* ICMP başlığı */
typedef struct {
    uint8_t  type;
    uint8_t  code;
    uint16_t checksum;
    uint16_t id;
    uint16_t seq;
} __attribute__((packed)) icmp_header_t;

/* Ağ durumu */
typedef struct {
    mac_addr_t mac;
    ip_addr_t  ip;
    ip_addr_t  gateway;
    ip_addr_t  subnet;
    bool       link_up;
    uint32_t   packets_sent;
    uint32_t   packets_recv;
    uint32_t   ping_ms;
    bool       nic_found;
} net_status_t;

#endif
