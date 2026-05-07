/*
Process A → tun0 → kernel(ns1) → veth → kernel(ns2) → tun1 → Process B

sudo bash setup_netns.sh

2. Compile
gcc proto_kernel_test.c -o proto_kernel_test
3. Run (same namespace setup as before)
Terminal 1 (receiver)
sudo ip netns exec ns2 ./proto_kernel_test B
Terminal 2 (sender)
sudo ip netns exec ns1 ./proto_kernel_test A
*/

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

/* ========================= */
/* Minimal mbuf replacement  */
/* ========================= */
struct mbuf {
    int len;
    unsigned char data[1500];
};

/* ========================= */
/* ifnet replacement         */
/* ========================= */
struct ifnet {
    int tun_fd;
    char name[16];
};

/* ========================= */
/* protocol instance         */
/* ========================= */
struct proto_instance {
    struct ifnet ifp;
    char name[16];
};

/* ========================= */
/* TUN allocation            */
/* ========================= */
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

/* ========================= */
/* checksum                  */
/* ========================= */
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

/* ========================= */
/* proto_output (TX)         */
/* ========================= */
int proto_output(struct proto_instance *inst, struct mbuf *m)
{
    printf("[%s] proto_output: sending %d bytes\n",
           inst->name, m->len);

    int rc = write(inst->ifp.tun_fd, m->data, m->len);
    if (rc < 0) {
        perror("write tun");
        return -1;
    }

    return 0;
}

/* ========================= */
/* proto_input (RX)          */
/* ========================= */
void proto_input(struct proto_instance *inst, struct mbuf *m)
{
    struct iphdr *ip = (struct iphdr*)m->data;

    printf("[%s] proto_input: len=%d src=%s dst=%s\n",
           inst->name,
           m->len,
           inet_ntoa(*(struct in_addr*)&ip->saddr),
           inet_ntoa(*(struct in_addr*)&ip->daddr));

    char *payload = (char*)m->data + sizeof(struct iphdr);

    printf("[%s] payload: %.4s\n", inst->name, payload);

    if (memcmp(payload, "TEST", 4) == 0) {
        printf("[%s] TEST PASSED\n", inst->name);
    } else {
        printf("[%s] TEST FAILED\n", inst->name);
    }
}

/* ========================= */
/* RX loop                   */
/* ========================= */
void rx_loop(struct proto_instance *inst)
{
    struct mbuf m;

    while (1) {
        int n = read(inst->ifp.tun_fd, m.data, sizeof(m.data));

        if (n <= 0)
            continue;

        m.len = n;

        struct iphdr *ip = (struct iphdr*)m.data;

        /* filter only our traffic */
        if (ip->saddr != inet_addr("10.0.0.99"))
            continue;

        proto_input(inst, &m);
        break;
    }
}

/* ========================= */
/* build packet              */
/* ========================= */
void build_packet(struct mbuf *m)
{
    struct iphdr *ip = (struct iphdr*)m->data;

    memset(m, 0, sizeof(*m));

    ip->ihl = 5;
    ip->version = 4;
    ip->ttl = 64;
    ip->protocol = IPPROTO_ICMP;

    ip->saddr = inet_addr("10.0.0.99");
    ip->daddr = inet_addr("10.0.1.99");

    char *payload = (char*)m->data + sizeof(struct iphdr);
    memcpy(payload, "TEST", 4);

    ip->tot_len = htons(sizeof(struct iphdr) + 4);
    ip->check = csum(ip, sizeof(struct iphdr));

    m->len = sizeof(struct iphdr) + 4;
}

/* ========================= */
/* MAIN                      */
/* ========================= */
int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("Usage: %s A|B\n", argv[0]);
        return 1;
    }

    struct proto_instance inst;

    if (argv[1][0] == 'A') {
        strcpy(inst.name, "INSTANCE_A");
        strcpy(inst.ifp.name, "tun0");

        inst.ifp.tun_fd = tun_alloc("tun0");

        system("ip addr replace 10.0.0.1/24 dev tun0");
        system("ip link set tun0 up");
        system("ip route replace 10.0.1.0/24 via 192.168.0.2 dev veth0");

        sleep(2);

        struct mbuf m;
        build_packet(&m);

        proto_output(&inst, &m);
    }

    else {
        strcpy(inst.name, "INSTANCE_B");
        strcpy(inst.ifp.name, "tun1");

        inst.ifp.tun_fd = tun_alloc("tun1");

        system("ip addr replace 10.0.1.1/24 dev tun1");
        system("ip link set tun1 up");
        system("ip route replace 10.0.0.0/24 via 192.168.0.1 dev veth1");

        rx_loop(&inst);
    }

    return 0;
}