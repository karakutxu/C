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

    int fd = socket(AF_PACKET, SOCK_RAW,
                    htons(CUSTOM_ETHERTYPE));

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

    if (bind(fd, (struct sockaddr *)&sll,
             sizeof(sll)) < 0) {
        perror("bind");
        exit(1);
    }

    return fd;
}

static void get_mac(int fd,
                    char *iface,
                    unsigned char *mac)
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

static void tx_loop(int tun_fd,
                    int raw_fd,
                    unsigned char *src_mac,
                    unsigned char *dst_mac)
{
    unsigned char ipbuf[BUF_SIZE];
    unsigned char frame[BUF_SIZE];

    while (1) {

        int n = read(tun_fd, ipbuf, sizeof(ipbuf));

        if (n < 0) {
            perror("read tun");
            continue;
        }

        struct ether_header *eth =
            (struct ether_header *)frame;

        memcpy(eth->ether_dhost, dst_mac, 6);
        memcpy(eth->ether_shost, src_mac, 6);

        eth->ether_type =
            htons(CUSTOM_ETHERTYPE);

        memcpy(frame + sizeof(struct ether_header),
               ipbuf,
               n);

        int txlen =
            n + sizeof(struct ether_header);

        if (write(raw_fd, frame, txlen) < 0) {
            perror("write raw");
            continue;
        }

        struct iphdr *ip =
            (struct iphdr *)ipbuf;

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

static void rx_loop(int tun_fd,
                    int raw_fd)
{
    unsigned char frame[BUF_SIZE];

    while (1) {

        int n = read(raw_fd,
                     frame,
                     sizeof(frame));

        if (n < 0) {
            perror("read raw");
            continue;
        }

        struct ether_header *eth =
            (struct ether_header *)frame;

        if (ntohs(eth->ether_type) !=
            CUSTOM_ETHERTYPE)
            continue;

        unsigned char *ip_pkt =
            frame + sizeof(struct ether_header);

        int ip_len =
            n - sizeof(struct ether_header);

        if (write(tun_fd,
                  ip_pkt,
                  ip_len) < 0) {

            perror("write tun");
            continue;
        }

        struct iphdr *ip =
            (struct iphdr *)ip_pkt;

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

        printf("\nUsage:\n");

        printf("%s tx <tun> <eth> <dst-mac>\n",
               argv[0]);

        printf("%s rx <tun> <eth> dummy\n",
               argv[0]);

        return 1;
    }

    char *mode = argv[1];
    char *tun_name = argv[2];
    char *eth_name = argv[3];

    int tun_fd = tun_alloc(tun_name);

    int raw_fd =
        raw_socket_open(eth_name);

    if (!strcmp(mode, "tx")) {

        unsigned char src_mac[6];
        unsigned char dst_mac[6];

        get_mac(raw_fd,
                eth_name,
                src_mac);

        sscanf(argv[4],
               "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
               &dst_mac[0],
               &dst_mac[1],
               &dst_mac[2],
               &dst_mac[3],
               &dst_mac[4],
               &dst_mac[5]);

        tx_loop(tun_fd,
                raw_fd,
                src_mac,
                dst_mac);
    }
    else {

        rx_loop(tun_fd,
                raw_fd);
    }

    return 0;
}
