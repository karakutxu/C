#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <arpa/inet.h>
#include <netinet/ip.h>

/* create tun */
int tun_alloc(char *dev)
{
    struct ifreq ifr;
    int fd = open("/dev/net/tun", O_RDWR);

    if (fd < 0) {
        perror("open tun");
        exit(1);
    }

    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
    strcpy(ifr.ifr_name, dev);

    if (ioctl(fd, TUNSETIFF, &ifr) < 0) {
        perror("TUNSETIFF");
        exit(1);
    }

    return fd;
}

/* checksum */
unsigned short csum(void *buf, int len)
{
    unsigned short *data = buf;
    unsigned int sum = 0;

    while (len > 1) {
        sum += *data++;
        len -= 2;
    }

    if (len)
        sum += *(unsigned char*)data;

    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);

    return ~sum;
}

/* build packet */
int build_packet(char *buf)
{
    struct iphdr *ip = (struct iphdr*)buf;

    memset(buf, 0, 1500);

    ip->ihl = 5;
    ip->version = 4;
    ip->ttl = 64;
    ip->protocol = IPPROTO_ICMP;

    ip->saddr = inet_addr("10.0.0.99");
    ip->daddr = inet_addr("10.0.1.99");

    ip->tot_len = htons(sizeof(struct iphdr) + 4);

    char *payload = buf + sizeof(struct iphdr);
    memcpy(payload, "TEST", 4);

    ip->check = csum(ip, sizeof(struct iphdr));

    return sizeof(struct iphdr) + 4;
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("Usage: %s [A|B]\n", argv[0]);
        return 1;
    }

    char *mode = argv[1];

    if (mode[0] == 'A') {
        /* namespace ns1 */
        int fd = tun_alloc("tun0");

        system("ip addr add 10.0.0.1/24 dev tun0");
        system("ip link set tun0 up");

        system("ip route add 10.0.1.0/24 via 192.168.0.2 dev veth0");

        sleep(2);

        char buf[1500];
        int len = build_packet(buf);

        printf("TX: sending packet\n");
        write(fd, buf, len);

        printf("TX done\n");
    }

    else if (mode[0] == 'B') {
        /* namespace ns2 */
        int fd = tun_alloc("tun1");

        system("ip addr add 10.0.1.1/24 dev tun1");
        system("ip link set tun1 up");

        system("ip route add 10.0.0.0/24 via 192.168.0.1 dev veth1");

        char buf[2000];

        printf("RX: waiting...\n");

        while (1) {
            int n = read(fd, buf, sizeof(buf));

            struct iphdr *ip = (struct iphdr*)buf;

            if (ip->saddr != inet_addr("10.0.0.99"))
                continue;

            printf("RX: got packet len=%d\n", n);

            char *payload = buf + sizeof(struct iphdr);

            printf("payload: %.4s\n", payload);

            if (memcmp(payload, "TEST", 4) == 0) {
                printf("TEST PASSED\n");
                break;
            }
        }
    }

    return 0;
}
