#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <sys/ioctl.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#include <linux/if.h>
#include <linux/if_tun.h>

#include <netinet/ip.h>

#define BUF_SIZE 2000
#define PORT 5555

static int tun_alloc(char *dev)
{
    struct ifreq ifr;

    int fd = open("/dev/net/tun", O_RDWR);

    if (fd < 0) {
        perror("open tun");
        exit(1);
    }

    memset(&ifr, 0, sizeof(ifr));

    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;

    strncpy(ifr.ifr_name, dev, IFNAMSIZ);

    if (ioctl(fd, TUNSETIFF, &ifr) < 0) {
        perror("TUNSETIFF");
        exit(1);
    }

    return fd;
}

static int udp_bind_socket(int port)
{
    int fd;

    struct sockaddr_in addr;

    fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (fd < 0) {
        perror("socket");
        exit(1);
    }

    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(fd,
             (struct sockaddr *)&addr,
             sizeof(addr)) < 0) {
        perror("bind");
        exit(1);
    }

    return fd;
}

static void tx_loop(int tun_fd,
                    int udp_fd,
                    char *peer_ip)
{
    unsigned char buf[BUF_SIZE];

    struct sockaddr_in peer;

    memset(&peer, 0, sizeof(peer));

    peer.sin_family = AF_INET;
    peer.sin_port = htons(PORT);

    inet_pton(AF_INET,
              peer_ip,
              &peer.sin_addr);

    while (1) {

        int n = read(tun_fd,
                     buf,
                     sizeof(buf));

        if (n < 0) {
            perror("read tun");
            continue;
        }

        if (sendto(udp_fd,
                   buf,
                   n,
                   0,
                   (struct sockaddr *)&peer,
                   sizeof(peer)) < 0) {

            perror("sendto");
            continue;
        }

        struct iphdr *ip =
            (struct iphdr *)buf;

        char srcbuf[64];
        char dstbuf[64];

        struct in_addr saddr, daddr;

        saddr.s_addr = ip->saddr;
        daddr.s_addr = ip->daddr;

        inet_ntop(AF_INET,
                  &saddr,
                  srcbuf,
                  sizeof(srcbuf));

        inet_ntop(AF_INET,
                  &daddr,
                  dstbuf,
                  sizeof(dstbuf));

        printf("TX: %s -> %s len=%d\n",
               srcbuf,
               dstbuf,
               n);
    }
}

static void rx_loop(int tun_fd,
                    int udp_fd)
{
    unsigned char buf[BUF_SIZE];

    while (1) {

        int n = recvfrom(udp_fd,
                         buf,
                         sizeof(buf),
                         0,
                         NULL,
                         NULL);

        if (n < 0) {
            perror("recvfrom");
            continue;
        }

        if (write(tun_fd,
                  buf,
                  n) < 0) {

            perror("write tun");
            continue;
        }

        struct iphdr *ip =
            (struct iphdr *)buf;

        char srcbuf[64];
        char dstbuf[64];

        struct in_addr saddr, daddr;

        saddr.s_addr = ip->saddr;
        daddr.s_addr = ip->daddr;

        inet_ntop(AF_INET,
                  &saddr,
                  srcbuf,
                  sizeof(srcbuf));

        inet_ntop(AF_INET,
                  &daddr,
                  dstbuf,
                  sizeof(dstbuf));

        printf("RX: %s -> %s len=%d\n",
               srcbuf,
               dstbuf,
               n);
    }
}

int main(int argc, char *argv[])
{
    if (argc != 4) {

        printf("\nUsage:\n");

        printf("%s tx <tun> <peer-ip>\n",
               argv[0]);

        printf("%s rx <tun> dummy\n",
               argv[0]);

        return 1;
    }

    char *mode = argv[1];
    char *tun_name = argv[2];

    int tun_fd = tun_alloc(tun_name);

    int udp_fd = udp_bind_socket(PORT);

    if (!strcmp(mode, "tx")) {

        tx_loop(tun_fd,
                udp_fd,
                argv[3]);
    }
    else {

        rx_loop(tun_fd,
                udp_fd);
    }

    return 0;
}

/*
having checked the legacy code, I have not seen anywhere references to AF_PACKET. However, there are numerous instances of AF_INET as below. The vast majority are SOCK_DGRAM type. socket(AF_INET, SOCK_RAW, IPPROTO_ICMP) socket(AF_INET, SOCK_DGRAM, 0) Does this mean that the legacy uses only raw IP sockets (IP protocol encapsulation), or raw IP sockets and UDP. Having made this clarification, Can I still use the same proto_demo.c with minimal changes? If so, can you please update proto_demo.c for me?
Yes — this changes the interpretation of the legacy architecture significantly, and your observation is extremely important.
Your legacy code is probably NOT using:
* raw Ethernet framing 
* custom EtherTypes 
* Layer-2 packet injection 
despite the internal L2_addr naming.
Instead, it appears much closer to:
custom Layer-3 overlay transport
over IP/UDP sockets
which is actually GOOD news for your Linux port.

What the legacy code is likely doing
The important clue is:
socket(AF_INET, SOCK_DGRAM, 0)
and:
socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)
This strongly suggests the transport layer is:
* IPv4-based 
* probably UDP-based for data transport 
* possibly raw IP only for special protocols like ICMP 
NOT Ethernet framing.

What this means architecturally
Your earlier assumption:
proto_output()
   ?
Ethernet frame transport
was probably incorrect.
The real architecture is likely:
proto_output()
   ?
custom scheduling / queueing
   ?
UDP/IP transport
   ?
network
   ?
UDP/IP receive
   ?
proto_input()
This is MUCH easier to port.

Why the code still looked Layer-2-ish
Because your driver maintains:
* logical node addresses 
* flow routing 
* next-hop identifiers 
using terminology like:
* L2_addr 
* subflow 
* link_stat 
But that does NOT necessarily mean Ethernet.
Many legacy embedded protocols:
* implemented their own logical overlay addressing 
* while using IP/UDP underneath 
That is very common.

This changes the Linux recommendation
You probably do NOT want:
AF_PACKET
at all.
Instead you likely want:
TUN ? userspace daemon ? UDP sockets ? network ? UDP sockets ? userspace daemon ? TUN
This is MUCH closer to the original system.

HUGE advantage
Now interoperability becomes dramatically easier.
Because:
* old system already speaks IP/UDP 
* Linux already speaks IP/UDP 
* routing already exists 
* switches already exist 
* NICs already exist 
You no longer need:
* custom EtherTypes 
* raw Ethernet 
* AF_PACKET 
* MAC management 

This is probably the REAL architecture
Linux kernel
    ?
TUN
    ?
userspace protocol daemon
    ?
UDP socket
    ?
IP network
    ?
UDP socket
    ?
userspace protocol daemon
    ?
TUN
    ?
Linux kernel
That is a VERY clean migration path.

Therefore YES
You can absolutely keep the same proto_demo.c
with only MINIMAL changes.
You only replace:
AF_PACKET raw Ethernet transport
with:
UDP transport
Everything else remains identical:
* TUN model 
* read(tun) 
* write(tun) 
* userspace routing 
* packet ownership 

This is actually closer to your legacy implementation
Because your legacy system likely already depended on:
* IP routing 
* UDP/IP delivery 
* IP connectivity 
rather than raw Ethernet delivery.

Updated architecture
OLD demo
TUN
 ?
AF_PACKET
 ?
Ethernet

NEW corrected demo
TUN
 ?
UDP socket
 ?
IP network
This is probably the correct migration target.

Why this is better
This immediately gives:
* multi-hop routing 
* WAN support 
* NAT traversal possibilities 
* easier interoperability 
* easier debugging 
* easier deployment 
* no root-only AF_PACKET dependency 
and matches your legacy socket usage.

Minimal-change updated proto_demo.c
The ONLY thing changing is:
* replace raw Ethernet sockets
with: 
* UDP sockets 
The TUN logic remains identical.

New flow
TX
read(tun)
   ?
sendto(udp)

RX
recvfrom(udp)
    ?
write(tun)

Updated compile-ready version
Replace the old proto_demo.c entirely with this:
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <sys/ioctl.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#include <linux/if.h>
#include <linux/if_tun.h>

#include <netinet/ip.h>

#define BUF_SIZE 2000
#define PORT 5555

static int tun_alloc(char *dev)
{
    struct ifreq ifr;

    int fd = open("/dev/net/tun", O_RDWR);

    if (fd < 0) {
        perror("open tun");
        exit(1);
    }

    memset(&ifr, 0, sizeof(ifr));

    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;

    strncpy(ifr.ifr_name, dev, IFNAMSIZ);

    if (ioctl(fd, TUNSETIFF, &ifr) < 0) {
        perror("TUNSETIFF");
        exit(1);
    }

    return fd;
}

static int udp_bind_socket(int port)
{
    int fd;

    struct sockaddr_in addr;

    fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (fd < 0) {
        perror("socket");
        exit(1);
    }

    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(fd,
             (struct sockaddr *)&addr,
             sizeof(addr)) < 0) {
        perror("bind");
        exit(1);
    }

    return fd;
}

static void tx_loop(int tun_fd,
                    int udp_fd,
                    char *peer_ip)
{
    unsigned char buf[BUF_SIZE];

    struct sockaddr_in peer;

    memset(&peer, 0, sizeof(peer));

    peer.sin_family = AF_INET;
    peer.sin_port = htons(PORT);

    inet_pton(AF_INET,
              peer_ip,
              &peer.sin_addr);

    while (1) {

        int n = read(tun_fd,
                     buf,
                     sizeof(buf));

        if (n < 0) {
            perror("read tun");
            continue;
        }

        if (sendto(udp_fd,
                   buf,
                   n,
                   0,
                   (struct sockaddr *)&peer,
                   sizeof(peer)) < 0) {

            perror("sendto");
            continue;
        }

        struct iphdr *ip =
            (struct iphdr *)buf;

        char srcbuf[64];
        char dstbuf[64];

        struct in_addr saddr, daddr;

        saddr.s_addr = ip->saddr;
        daddr.s_addr = ip->daddr;

        inet_ntop(AF_INET,
                  &saddr,
                  srcbuf,
                  sizeof(srcbuf));

        inet_ntop(AF_INET,
                  &daddr,
                  dstbuf,
                  sizeof(dstbuf));

        printf("TX: %s -> %s len=%d\n",
               srcbuf,
               dstbuf,
               n);
    }
}

static void rx_loop(int tun_fd,
                    int udp_fd)
{
    unsigned char buf[BUF_SIZE];

    while (1) {

        int n = recvfrom(udp_fd,
                         buf,
                         sizeof(buf),
                         0,
                         NULL,
                         NULL);

        if (n < 0) {
            perror("recvfrom");
            continue;
        }

        if (write(tun_fd,
                  buf,
                  n) < 0) {

            perror("write tun");
            continue;
        }

        struct iphdr *ip =
            (struct iphdr *)buf;

        char srcbuf[64];
        char dstbuf[64];

        struct in_addr saddr, daddr;

        saddr.s_addr = ip->saddr;
        daddr.s_addr = ip->daddr;

        inet_ntop(AF_INET,
                  &saddr,
                  srcbuf,
                  sizeof(srcbuf));

        inet_ntop(AF_INET,
                  &daddr,
                  dstbuf,
                  sizeof(dstbuf));

        printf("RX: %s -> %s len=%d\n",
               srcbuf,
               dstbuf,
               n);
    }
}

int main(int argc, char *argv[])
{
    if (argc != 4) {

        printf("\nUsage:\n");

        printf("%s tx <tun> <peer-ip>\n",
               argv[0]);

        printf("%s rx <tun> dummy\n",
               argv[0]);

        return 1;
    }

    char *mode = argv[1];
    char *tun_name = argv[2];

    int tun_fd = tun_alloc(tun_name);

    int udp_fd = udp_bind_socket(PORT);

    if (!strcmp(mode, "tx")) {

        tx_loop(tun_fd,
                udp_fd,
                argv[3]);
    }
    else {

        rx_loop(tun_fd,
                udp_fd);
    }

    return 0;
}

This is now probably VERY close to your legacy system
Because:
* IP routing remains external 
* your daemon owns packet scheduling 
* your daemon owns forwarding 
* your daemon owns queueing 
* transport is UDP/IP 
which matches the socket usage patterns you found.

*/