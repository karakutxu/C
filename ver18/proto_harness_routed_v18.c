/*
Build
gcc -O2 -Wall proto_harness_routed.c -o proto_harness_routed
Run

Terminal 1:

sudo ./setup_routed_netns.sh

Terminal 2:

sudo ip netns exec ns2 ./proto_harness_routed B

Terminal 3:

sudo ip netns exec ns1 ./proto_harness_routed A
Observe Real Routing

Now you can observe:

ns1 egress
sudo ip netns exec ns1 tcpdump -n -i veth1
router ingress/egress
sudo ip netns exec nsr tcpdump -n -i vethr1
sudo ip netns exec nsr tcpdump -n -i vethr2
ns2 ingress
sudo ip netns exec ns2 tcpdump -n -i veth2

You should now see:

packets traversing router namespace
real Linux forwarding
routed multi-subnet traffic
actual kernel routing path exercised

This is now a genuine routed kernel-assisted validation harness.
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <arpa/inet.h>

#include <sys/socket.h>
#include <netinet/in.h>

#define MAX_PAYLOAD   256
#define UDP_PORT      5555
#define MAGIC         0x48414c4f

#define IFNAMSIZ      16

#define IFF_UP        0x1
#define IFF_RUNNING   0x2

/*
 * BSD-like structures
 */

struct mbuf_pkthdr {

    uint32_t len;

    void *rcvif;
};

struct mbuf {

    uint8_t *m_data;

    uint8_t *m_datastart;

    int m_len;

    struct mbuf *m_next;

    struct mbuf *m_nextpkt;

    unsigned int m_flags;

    struct mbuf_pkthdr m_pkthdr;
};

struct ifnet {

    char if_name[IFNAMSIZ];

    int if_flags;

    int sockfd;

    void *p;

    uint64_t if_ibytes;

    uint64_t if_obytes;
};

struct proto_statics {

    char drvr_name[32];
};

struct proto_instance {

    struct ifnet ifp;

    struct proto_statics stats;
};

/*
 * deterministic test payload
 */

struct test_packet {

    uint32_t magic;

    uint32_t seq;

    uint32_t payload_len;

    uint32_t checksum;

    uint8_t payload[MAX_PAYLOAD];
};

static uint32_t checksum32(uint8_t *buf,
                           size_t len)
{
    uint32_t sum = 0;

    for (size_t i = 0; i < len; i++)
        sum += buf[i];

    return sum;
}

static struct mbuf *mbuf_alloc(size_t len)
{
    struct mbuf *m;

    m = calloc(1, sizeof(*m));

    if (!m)
        return NULL;

    m->m_datastart = calloc(1, len);

    if (!m->m_datastart) {

        free(m);

        return NULL;
    }

    m->m_data = m->m_datastart;

    m->m_len = len;

    m->m_pkthdr.len = len;

    return m;
}

static void mbuf_free(struct mbuf *m)
{
    if (!m)
        return;

    free(m->m_datastart);

    free(m);
}

static void build_packet(struct test_packet *pkt,
                         uint32_t seq)
{
    memset(pkt, 0, sizeof(*pkt));

    pkt->magic = MAGIC;

    pkt->seq = seq;

    pkt->payload_len = MAX_PAYLOAD;

    for (uint32_t i = 0;
         i < MAX_PAYLOAD;
         i++) {

        pkt->payload[i] =
            (uint8_t)((seq + i) & 0xff);
    }

    pkt->checksum =
        checksum32(pkt->payload,
                   pkt->payload_len);
}

static int validate_packet(struct test_packet *pkt)
{
    uint32_t calc;

    if (pkt->magic != MAGIC) {

        printf("BAD MAGIC 0x%x\n",
               pkt->magic);

        return -1;
    }

    calc = checksum32(pkt->payload,
                      pkt->payload_len);

    printf("checksum rx=%u calc=%u\n",
           pkt->checksum,
           calc);

    if (calc != pkt->checksum) {

        printf("CHECKSUM FAILURE\n");

        return -1;
    }

    for (uint32_t i = 0;
         i < pkt->payload_len;
         i++) {

        uint8_t expected =
            (uint8_t)((pkt->seq + i) & 0xff);

        if (pkt->payload[i] != expected) {

            printf("PAYLOAD FAILURE idx=%u\n",
                   i);

            return -1;
        }
    }

    return 0;
}

/*
 * legacy semantic:
 *
 * proto_input()
 *   ->
 * inject INTO stack
 */

static int proto_input(struct ifnet *ifp,
                       struct mbuf *m,
                       const char *dst_ip)
{
    struct sockaddr_in dst;

    ssize_t n;

    memset(&dst, 0, sizeof(dst));

    dst.sin_family = AF_INET;

    dst.sin_port = htons(UDP_PORT);

    inet_pton(AF_INET,
              dst_ip,
              &dst.sin_addr);

    n = sendto(ifp->sockfd,
               m->m_data,
               m->m_len,
               0,
               (struct sockaddr *)&dst,
               sizeof(dst));

    if (n < 0) {

        perror("sendto");

        return -1;
    }

    ifp->if_obytes += n;

    printf("\n=== proto_input() ===\n");

    printf("Injected %ld bytes into kernel stack\n",
           n);

    return 0;
}

/*
 * legacy semantic:
 *
 * stack
 *   ->
 * proto_output()
 */

static void proto_output(struct ifnet *ifp,
                         struct mbuf *m)
{
    struct test_packet *pkt;

    pkt = (struct test_packet *)m->m_data;

    printf("\n=== proto_output() ===\n");

    printf("if=%s len=%d seq=%u\n",
           ifp->if_name,
           m->m_len,
           pkt->seq);

    validate_packet(pkt);

    ifp->if_ibytes += m->m_len;

    mbuf_free(m);
}

static int create_socket(const char *bind_ip)
{
    int fd;

    struct sockaddr_in addr;

    fd = socket(AF_INET,
                SOCK_DGRAM,
                0);

    if (fd < 0) {

        perror("socket");

        exit(1);
    }

    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;

    addr.sin_port = htons(UDP_PORT);

    inet_pton(AF_INET,
              bind_ip,
              &addr.sin_addr);

    if (bind(fd,
             (struct sockaddr *)&addr,
             sizeof(addr)) < 0) {

        perror("bind");

        exit(1);
    }

    return fd;
}

static void sender_loop(struct proto_instance *pi)
{
    uint32_t seq = 1;

    while (1) {

        struct mbuf *m;

        struct test_packet *pkt;

        m = mbuf_alloc(sizeof(*pkt));

        pkt = (struct test_packet *)m->m_data;

        build_packet(pkt, seq++);

        proto_input(&pi->ifp,
                    m,
                    "10.0.2.1");

        mbuf_free(m);

        sleep(1);
    }
}

static void receiver_loop(struct proto_instance *pi)
{
    while (1) {

        uint8_t buffer[2048];

        ssize_t n;

        struct sockaddr_in src;

        socklen_t slen = sizeof(src);

        struct mbuf *m;

        n = recvfrom(pi->ifp.sockfd,
                     buffer,
                     sizeof(buffer),
                     0,
                     (struct sockaddr *)&src,
                     &slen);

        if (n < 0) {

            perror("recvfrom");

            continue;
        }

        printf("\nRX from %s len=%ld\n",
               inet_ntoa(src.sin_addr),
               n);

        if ((size_t)n < sizeof(struct test_packet)) {

            printf("DROP short packet\n");

            continue;
        }

        m = mbuf_alloc(n);

        memcpy(m->m_data,
               buffer,
               n);

        proto_output(&pi->ifp,
                     m);
    }
}

int main(int argc,
         char **argv)
{
    struct proto_instance pi;

    memset(&pi, 0, sizeof(pi));

    if (argc < 2) {

        printf("usage:\n");

        printf("sender   : %s A\n",
               argv[0]);

        printf("receiver : %s B\n",
               argv[0]);

        return 1;
    }

    strcpy(pi.stats.drvr_name,
           "PROTO");

    strcpy(pi.ifp.if_name,
           "proto0");

    pi.ifp.if_flags =
        IFF_UP | IFF_RUNNING;

    if (strcmp(argv[1], "A") == 0) {

        pi.ifp.sockfd =
            create_socket("10.0.1.1");

        printf("Sender ready\n");

        sender_loop(&pi);
    }
    else {

        pi.ifp.sockfd =
            create_socket("10.0.2.1");

        printf("Receiver ready\n");

        receiver_loop(&pi);
    }

    return 0;
}