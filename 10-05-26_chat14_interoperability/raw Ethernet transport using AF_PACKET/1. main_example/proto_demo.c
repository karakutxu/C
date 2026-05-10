
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
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <net/ethernet.h>
#include <netinet/ip.h>

#define CUSTOM_ETHERTYPE 0x88B5
#define BUF_SIZE 2000

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

static int raw_socket_open(char *iface)
{
    struct ifreq ifr;
    struct sockaddr_ll sll;

    int fd = socket(AF_PACKET, SOCK_RAW, htons(CUSTOM_ETHERTYPE));
    if (fd < 0) {
        perror("socket");
        exit(1);
    }

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, iface, IFNAMSIZ);

    if (ioctl(fd, SIOCGIFINDEX, &ifr) < 0) {
        perror("SIOCGIFINDEX");
        exit(1);
    }

    memset(&sll, 0, sizeof(sll));
    sll.sll_family = AF_PACKET;
    sll.sll_protocol = htons(CUSTOM_ETHERTYPE);
    sll.sll_ifindex = ifr.ifr_ifindex;

    if (bind(fd, (struct sockaddr *)&sll, sizeof(sll)) < 0) {
        perror("bind");
        exit(1);
    }

    return fd;
}

static void get_mac(int fd, char *iface, unsigned char *mac)
{
    struct ifreq ifr;

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, iface, IFNAMSIZ);

    if (ioctl(fd, SIOCGIFHWADDR, &ifr) < 0) {
        perror("SIOCGIFHWADDR");
        exit(1);
    }

    memcpy(mac, ifr.ifr_hwaddr.sa_data, 6);
}

static void tx_loop(int tun_fd, int raw_fd,
                    unsigned char *src_mac,
                    unsigned char *dst_mac)
{
    unsigned char buf[BUF_SIZE];
    unsigned char frame[BUF_SIZE];

    while (1) {

        int n = read(tun_fd, buf, sizeof(buf));
        if (n < 0) {
            perror("read tun");
            continue;
        }

        struct ether_header *eth = (struct ether_header *)frame;

        memcpy(eth->ether_dhost, dst_mac, 6);
        memcpy(eth->ether_shost, src_mac, 6);
        eth->ether_type = htons(CUSTOM_ETHERTYPE);

        memcpy(frame + sizeof(struct ether_header), buf, n);

        int txlen = n + sizeof(struct ether_header);

        if (write(raw_fd, frame, txlen) < 0) {
            perror("write raw");
            continue;
        }

        struct iphdr *ip = (struct iphdr *)buf;
        struct in_addr saddr, daddr;

        saddr.s_addr = ip->saddr;
        daddr.s_addr = ip->daddr;

		char srcbuf[64];
		char dstbuf[64];

		inet_ntop(AF_INET, &saddr, srcbuf, sizeof(srcbuf));
		inet_ntop(AF_INET, &daddr, dstbuf, sizeof(dstbuf));

		printf("TX: %s -> %s len=%d\n",
			   srcbuf,
			   dstbuf,
			   n);
    }
}

static void rx_loop(int tun_fd, int raw_fd)
{
    unsigned char frame[BUF_SIZE];

    while (1) {

        int n = read(raw_fd, frame, sizeof(frame));
        if (n < 0) {
            perror("read raw");
            continue;
        }

        struct ether_header *eth = (struct ether_header *)frame;

        if (ntohs(eth->ether_type) != CUSTOM_ETHERTYPE)
            continue;

        unsigned char *ip_pkt = frame + sizeof(struct ether_header);

        int ip_len = n - sizeof(struct ether_header);

        if (write(tun_fd, ip_pkt, ip_len) < 0) {
            perror("write tun");
            continue;
        }

        struct iphdr *ip = (struct iphdr *)ip_pkt;
        struct in_addr saddr, daddr;

        saddr.s_addr = ip->saddr;
        daddr.s_addr = ip->daddr;

		char srcbuf[64];
		char dstbuf[64];

		inet_ntop(AF_INET, &saddr, srcbuf, sizeof(srcbuf));
		inet_ntop(AF_INET, &daddr, dstbuf, sizeof(dstbuf));

		printf("TX: %s -> %s len=%d\n",
			   srcbuf,
			   dstbuf,
			   n);
    }
}

int main(int argc, char *argv[])
{
    if (argc != 5) {
        printf("Usage:\n");
        printf("%s tx <tun> <eth> <dst-mac>\n", argv[0]);
        printf("%s rx <tun> <eth> dummy\n", argv[0]);
        return 1;
    }

    char *mode = argv[1];
    char *tun_name = argv[2];
    char *eth_name = argv[3];

    int tun_fd = tun_alloc(tun_name);
    int raw_fd = raw_socket_open(eth_name);

    if (!strcmp(mode, "tx")) {

        unsigned char src_mac[6];
        unsigned char dst_mac[6];

        get_mac(raw_fd, eth_name, src_mac);

        sscanf(argv[4], "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
               &dst_mac[0],
               &dst_mac[1],
               &dst_mac[2],
               &dst_mac[3],
               &dst_mac[4],
               &dst_mac[5]);

        tx_loop(tun_fd, raw_fd, src_mac, dst_mac);
    }
    else {
        rx_loop(tun_fd, raw_fd);
    }

    return 0;
}

/*
 Summary
This C program creates a very simple Layer-2 tunnel between a Linux TUN interface and a raw Ethernet socket using a custom EtherType (0x88B5).
It operates in two modes:


TX mode (tx)
Reads IP packets from a TUN interface, wraps them in an Ethernet frame with a custom EtherType, and sends them over a physical Ethernet interface.


RX mode (rx)
Receives Ethernet frames with the custom EtherType from the Ethernet interface, strips the Ethernet header, and injects the IP packet into the TUN interface.


The result is effectively a lightweight custom Ethernet encapsulation mechanism for IP traffic.

High-Level Flow
TX Path
[TUN interface]      ↓Read raw IP packet      ↓Add Ethernet header      ↓Send via raw socket      ↓[Physical NIC]
RX Path
[Physical NIC]      ↓Receive Ethernet frame      ↓Check custom EtherType      ↓Remove Ethernet header      ↓Write IP packet into TUN      ↓[TUN interface]

Detailed Function Commentary

Headers and Constants
#define CUSTOM_ETHERTYPE 0x88B5#define BUF_SIZE 2000
Purpose


CUSTOM_ETHERTYPE


Defines the custom Ethernet protocol identifier.


Frames sent by this program use EtherType 0x88B5.




BUF_SIZE


Maximum packet/frame buffer size.




Notes


0x88B5 is in the experimental/local EtherType range.


2000 bytes is enough for standard MTU Ethernet frames.



tun_alloc()
static int tun_alloc(char *dev)
Purpose
Creates and configures a Linux TUN device.
What It Does


Opens /dev/net/tun


Requests:


IFF_TUN


TUN mode (IP packets only)




IFF_NO_PI


No extra packet metadata header






Assigns interface name


Returns file descriptor for packet I/O


Important Detail
TUN devices operate at Layer 3:


Reads/writes IP packets only


No Ethernet headers


Example
If interface is tun0:
ip tuntap add dev tun0 mode tun
Equivalent behavior is achieved programmatically here.

raw_socket_open()
static int raw_socket_open(char *iface)
Purpose
Creates a raw Ethernet socket bound to a physical interface.
What It Does


Creates:


socket(AF_PACKET, SOCK_RAW, htons(CUSTOM_ETHERTYPE))
This allows direct Ethernet frame access.


Retrieves interface index using:


SIOCGIFINDEX


Binds socket to:


specific NIC


specific EtherType




Result
The socket will:


send raw Ethernet frames


receive only frames with EtherType 0x88B5



get_mac()
static void get_mac(int fd, char *iface, unsigned char *mac)
Purpose
Obtains MAC address of the Ethernet interface.
Mechanism
Uses:
ioctl(... SIOCGIFHWADDR ...)
Then copies:
ifr.ifr_hwaddr.sa_data
into the supplied buffer.

tx_loop()
static void tx_loop(...)
Purpose
Encapsulates TUN IP packets inside Ethernet frames.

TX Processing Steps
1. Read packet from TUN
read(tun_fd, buf, sizeof(buf))
This yields a raw IP packet.

2. Construct Ethernet header
struct ether_header *eth
Sets:


destination MAC


source MAC


custom EtherType



3. Append IP payload
memcpy(frame + sizeof(struct ether_header), buf, n);
Resulting frame:
+-------------------+| Ethernet Header   |+-------------------+| IP Packet         |+-------------------+

4. Send frame
write(raw_fd, frame, txlen)
Sends directly at Layer 2.

5. Debug logging
Extracts IP header:
struct iphdr *ip = (struct iphdr *)buf;
Prints:
TX IP src -> dst len=N

rx_loop()
static void rx_loop(...)
Purpose
Receives Ethernet frames and injects IP packets into TUN.

RX Processing Steps
1. Receive Ethernet frame
read(raw_fd, frame, sizeof(frame))

2. Validate EtherType
if (ntohs(eth->ether_type) != CUSTOM_ETHERTYPE)    continue;
Filters unrelated traffic.

3. Remove Ethernet header
unsigned char *ip_pkt =    frame + sizeof(struct ether_header);

4. Inject packet into TUN
write(tun_fd, ip_pkt, ip_len)
The kernel now treats this as an incoming IP packet.

5. Debug logging
Prints source/destination IPs.

main()
Usage
program tx <tun> <eth> <dst-mac>program rx <tun> <eth> dummy

TX Example
./prog tx tun0 eth0 aa:bb:cc:dd:ee:ff


Reads packets from tun0


Sends Ethernet frames through eth0


Uses specified destination MAC



RX Example
./prog rx tun0 eth0 dummy


Receives custom EtherType frames from eth0


Injects packets into tun0



Architectural Notes
Why Use TUN?
TUN devices provide:


virtual Layer-3 interfaces


integration with Linux routing stack


This allows normal IP routing through the tunnel.

Why Raw Sockets?
Raw packet sockets allow:


direct Ethernet frame access


custom EtherType protocols


This bypasses TCP/UDP/IP transport layers.

Strengths
Simple Design
Very easy to understand:


TUN ↔ Ethernet bridge



Efficient
Minimal encapsulation:


only Ethernet header overhead



Good Learning Example
Demonstrates:


TUN/TAP programming


raw sockets


Ethernet framing


Linux networking internals



Weaknesses / Limitations
No Error Recovery
No:


retransmission


sequencing


acknowledgements


Packet loss is unmanaged.

No MTU Handling
Large packets may:


exceed Ethernet MTU


fragment unexpectedly



No Validation
Assumes:


packet contains valid IPv4


lengths are sane


Could crash or misbehave with malformed traffic.

Unsafe Parsing
Example:
struct iphdr *ip = (struct iphdr *)buf;
No length checks before accessing headers.
Should verify:
n >= sizeof(struct iphdr)

Uses Blocking I/O
Both loops block indefinitely.
Potential improvements:


select()


poll()


epoll()



No MAC Learning
Destination MAC must be manually provided.
No ARP or discovery mechanism exists.

No Security
Traffic is:


unencrypted


unauthenticated


Anyone on the LAN could inject fake frames.

Suggested Improvements
Add Length Validation
Example:
if (n < sizeof(struct ether_header))    continue;

Verify IPv4 Header
Check:


version


IHL


total length



Use Nonblocking I/O
Would improve responsiveness and scalability.

Add Signal Handling
Graceful cleanup on:


SIGINT


SIGTERM



Support Multiple Clients
Could maintain:


MAC table


peer list



Add Encryption
Could encapsulate:


AES-GCM


WireGuard-like framing



Add TAP Support
Using TAP instead of TUN would:


carry full Ethernet frames


preserve ARP/broadcast traffic



Overall Assessment
This is a concise and educational example of:


Linux TUN device programming


raw Ethernet sockets


custom EtherType protocols


packet encapsulation


Conceptually, it behaves like a very primitive Layer-2 VPN or Ethernet tunnel.
The code is suitable for:


experimentation


labs


protocol learning


controlled environments


But not production-ready due to:


lack of security


insufficient validation


no reliability handling


blocking architecture.

 
 */
