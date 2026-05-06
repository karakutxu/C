#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>

#include <linux/if_tun.h>
#include <net/if.h>
#include <netinet/ip.h>

/* ================= BASIC TYPES ================= */

typedef unsigned char  u_int_1;
typedef unsigned short u_int_2;
typedef unsigned int   u_int_4;
typedef int boolean_t;

#define true 1
#define false 0

/* ================= MBUF ================= */

struct mbuf_pkthdr {
    u_int_4 len;
    void *rcvif;
};

struct mbuf {
    u_int_1 *m_data;
    u_int_1 *m_datastart;
    int m_len;
    struct mbuf *m_next;
    struct mbuf *m_nextpkt;
    unsigned int m_flags;
    struct mbuf_pkthdr m_pkthdr;
};

#define mtod(m,t) ((t)((m)->m_data))

/* ================= IFNET ================= */

struct proto_statics {
    char drvr_name[16];
};

struct ifnet {
    int if_flags;
    void *p;
    int if_ibytes;
    int if_obytes;
};

/* ================= GLOBAL ================= */

#define MAX_INST 4
struct ifnet if_table[MAX_INST];
int inst_count = 0;

/* ================= TUN ================= */

int tun_fd;

int tun_alloc(char *dev)
{
    struct ifreq ifr;
    int fd = open("/dev/net/tun", O_RDWR);

    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
    strcpy(ifr.ifr_name, dev);

    ioctl(fd, TUNSETIFF, &ifr);
    return fd;
}

/* ================= MBUF HELPERS ================= */

struct mbuf *mbuf_alloc(int len)
{
    struct mbuf *m = malloc(sizeof(*m));
    m->m_datastart = malloc(len);
    m->m_data = m->m_datastart;
    m->m_len = len;
    m->m_pkthdr.len = len;
    m->m_next = NULL;
    m->m_nextpkt = NULL;
    return m;
}

void m_freem(struct mbuf *m)
{
    if (!m) return;
    free(m->m_datastart);
    free(m);
}

/* ================= ROUTING ================= */

int lookup_route(struct mbuf *m)
{
    struct ip *ip = mtod(m, struct ip *);

    if ((ntohl(ip->ip_dst.s_addr) & 0xFFFFFF00) == 0x0A000100)
        return 1; /* to B */

    if ((ntohl(ip->ip_dst.s_addr) & 0xFFFFFF00) == 0x0A000000)
        return 0; /* to A */

    return -1;
}

/* ================= DISPATCH ================= */

void proto_input(struct ifnet *ifp, struct mbuf *m);

void dispatcher_forward(struct ifnet *src, struct mbuf *m)
{
    int out = lookup_route(m);

    if (out < 0) {
        printf("DROP: no route\n");
        m_freem(m);
        return;
    }

    struct ifnet *dst = &if_table[out];

    printf("[DISPATCH] %s → %s\n",
        ((struct proto_statics*)src->p)->drvr_name,
        ((struct proto_statics*)dst->p)->drvr_name);

    proto_input(dst, m);
}

/* ================= PROTO_INPUT ================= */

void proto_input(struct ifnet *ifp, struct mbuf *m)
{
    struct proto_statics *s = ifp->p;

    if (!m) return;

    struct ip *ip = mtod(m, struct ip *);

    printf("[%s] proto_input: ", s->drvr_name);
	
	char src[INET_ADDRSTRLEN];
	char dst[INET_ADDRSTRLEN];

	inet_ntop(AF_INET, &ip->ip_src, src, sizeof(src));
	inet_ntop(AF_INET, &ip->ip_dst, dst, sizeof(dst));

	printf("%s → %s\n", src, dst);

    ifp->if_ibytes += m->m_pkthdr.len;
}

/* ================= PROTO_OUTPUT ================= */

int proto_output(struct ifnet *ifp, struct mbuf *m)
{
    struct proto_statics *s = ifp->p;

    if (!m) return -1;

    struct ip *ip = mtod(m, struct ip *);

	char dst[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &ip->ip_dst, dst, sizeof(dst));

	printf("[%s] proto_output: routing %s\n",
		s->drvr_name, dst);

    dispatcher_forward(ifp, m);

    ifp->if_obytes += m->m_pkthdr.len;

    return 0;
}

/* ================= TEST PACKET ================= */

struct mbuf *build_pkt(const char *src, const char *dst)
{
    struct mbuf *m = mbuf_alloc(sizeof(struct ip));

    struct ip *ip = mtod(m, struct ip *);

    memset(ip, 0, sizeof(*ip));
    ip->ip_v = 4;
    ip->ip_hl = 5;
    ip->ip_len = htons(sizeof(struct ip));
    ip->ip_ttl = 64;
    ip->ip_p = IPPROTO_UDP;

    ip->ip_src.s_addr = inet_addr(src);
    ip->ip_dst.s_addr = inet_addr(dst);

    return m;
}

/* ================= MAIN ================= */

int main()
{
    /* create instances */
    struct proto_statics *A = malloc(sizeof(*A));
    struct proto_statics *B = malloc(sizeof(*B));

    strcpy(A->drvr_name, "A");
    strcpy(B->drvr_name, "B");

    if_table[0].p = A;
    if_table[1].p = B;

    inst_count = 2;

    /* tests */

    printf("\n=== TEST A → B ===\n");
    struct mbuf *m1 = build_pkt("10.0.0.1", "10.0.1.1");
    proto_output(&if_table[0], m1);

    printf("\n=== TEST B → A ===\n");
    struct mbuf *m2 = build_pkt("10.0.1.1", "10.0.0.1");
    proto_output(&if_table[1], m2);

    /* TUN */

    tun_fd = tun_alloc("tun0");

    unsigned char buf[2000];

    printf("\n=== LIVE MODE ===\n");

    while (1) {
        int len = read(tun_fd, buf, sizeof(buf));
        if (len < 0) break;

        struct mbuf *m = mbuf_alloc(len);
        memcpy(m->m_data, buf, len);

        proto_input(&if_table[0], m);
        proto_output(&if_table[0], m);

        write(tun_fd, buf, len);
    }

    return 0;
}