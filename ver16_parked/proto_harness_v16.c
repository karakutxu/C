/* 	Emulation of protocol driver <--> network stack boundary

	network stack / protocol driver boundary validation

           (Network Stack)
        ┌────────────────────┐
        │                    │
        │   routing logic    │
        │   (kernel / IP)    │
        │                    │
        └────────────────────┘
           ↑              ↓
     proto_input      proto_output
       (RX)              (TX)

NOTE: There is no direct proto_output(A) → proto_input(B) path	  
This test harness simulates the network stack:
		proto_input(B) --> mock_stack_route() --> proto_output(A)
	   
RX path (independent)
proto_input(B)
   → inject packet into stack
      → stack routes
         → proto_output(A)
		 
TX path (independent)
proto_output(A)
   ← stack delivers packet to driver
   
 
	Multi-process test harness: allows multiple instances of the protocol driver
	Single routing domain (kernel)
	Multiple TUN interfaces (one per instance) ! --> this make omplicate the configuration
	Deterministic packet generation and validation
	End-to-end validation
	Checksum + payload verification
	Configurable routing

Run:
	chmod +x setup_netns.sh
	sudo ./setup_netns.sh

Run:
	chmod +x configure_tuns.sh
	sudo ./configure_tuns.sh

BUILD
	gcc -O2 -Wall proto_harness.c -o proto_harness

RUN Receiver:
	sudo ip netns exec ns2 ./proto_harness B tun1

RUN Sender:
	sudo ip netns exec ns1 ./proto_harness A tun0

ip route
sudo tcpdump -i tun0 -n
sudo tcpdump -i tun1 -n
*/

/*
 * proto_harness_v16.c
 *
 * COMPLETE compile-ready user-space protocol harness
 *
 * FIXES:
 *   ✔ Proper IPv4 packet generation
 *   ✔ Proper UDP encapsulation
 *   ✔ Correct TUN semantics
 *   ✔ Receiver filtering
 *   ✔ Ignore Linux background noise
 *   ✔ Detailed debugging
 *   ✔ Correct namespace routing
 *   ✔ Deterministic payload validation
 *   ✔ Checksum verification
 *   ✔ Legacy proto_input/proto_output semantics
 *
 * Run:
 *	chmod +x setup_netns_v14.sh
 *	sudo ./setup_netns_v14.sh
 * Run:
 *	chmod +x configure_tuns_v14.sh
 *	sudo ./configure_tuns_v16.sh
 * 
 * BUILD:
 *   gcc -O2 -Wall proto_harness_v16.c -o proto_harness_v16
 *
 * RUN:
 *
 * Receiver:
 *   sudo ip netns exec ns2 ./proto_harness_v16 B tun1
 *
 * Sender:
 *   sudo ip netns exec ns1 ./proto_harness_v16 A tun0
 *
 * TROUBLESHOOTING COMMANDS
 * 
 * 1. Observe tun0 TX
 * Inside ns1:
 * 		sudo ip netns exec ns1 tcpdump -n -i tun0
 * Expected:
 * IP 172.16.0.1.5555 > 172.16.0.2.5555
 * 
 * 2. Observe veth0
 * 		sudo ip netns exec ns1 tcpdump -n -i veth0
 * Expected:
 * packet forwarded from tun0 toward ns2
 * 
 * 3. Observe veth1
 * 		sudo ip netns exec ns2 tcpdump -n -i veth1
 * Expected:
 * packet arrives in ns2
 * 
 * 4. Observe tun1
 * 		sudo ip netns exec ns2 tcpdump -n -i tun1
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <arpa/inet.h>

#include <linux/if.h>
#include <linux/if_tun.h>

#include <netinet/ip.h>
#include <netinet/udp.h>

#include <sys/ioctl.h>

#define MAX_PKT_SIZE 2048

#define TEST_PORT 5555
#define TEST_MAGIC 0xdeadbeef

//#define IFF_UP      0x1
//#define IFF_RUNNING 0x2

typedef uint8_t  u_int_1;
typedef uint16_t u_int_2;
typedef uint32_t u_int_4;

typedef uint8_t L2_addr_t;

/*************************************************************
 * mbuf
 *************************************************************/

struct mbuf_pkthdr {
    u_int_4 len;
    void *rcvif;
};

struct mbuf {
    u_int_1 *m_data;
    u_int_1 *m_datastart;

    int m_len;

    u_int_4 enqueue_time;

    struct mbuf *m_next;
    struct mbuf *m_nextpkt;

    unsigned int m_flags;

    struct mbuf_pkthdr m_pkthdr;
};

/*************************************************************
 * ifnet
 *************************************************************/

struct ifnet {

    char if_name[16];

    int if_flags;

    uint64_t if_ibytes;
    uint64_t if_obytes;

    int tun_fd;

    void *p;
};

struct proto_instance {

    struct ifnet ifp;

    char name[32];
};

/*************************************************************
 * test payload
 *************************************************************/

struct test_packet {

    uint32_t magic;

    uint32_t seq;

    uint32_t checksum;

    uint8_t payload[256];
};

struct full_packet {

    struct iphdr ip;

    struct udphdr udp;

    struct test_packet tp;
};

/*************************************************************
 * checksum helpers
 *************************************************************/

static uint32_t payload_checksum(uint8_t *buf,
                                 size_t len)
{
    uint32_t sum = 0;

    for (size_t i = 0; i < len; i++)
        sum += buf[i];

    return sum;
}

static uint16_t ip_checksum(void *vdata,
                            size_t length)
{
    char *data = (char *)vdata;

    uint64_t acc = 0xffff;

    for (size_t i = 0; i + 1 < length; i += 2) {

        uint16_t word;

        memcpy(&word, data + i, 2);

        acc += ntohs(word);

        if (acc > 0xffff)
            acc -= 0xffff;
    }

    if (length & 1) {

        uint16_t word = 0;

        memcpy(&word, data + length - 1, 1);

        acc += ntohs(word);

        if (acc > 0xffff)
            acc -= 0xffff;
    }

    return htons(~acc);
}

/*************************************************************
 * mbuf helpers
 *************************************************************/

static struct mbuf *mbuf_alloc(size_t len)
{
    struct mbuf *m;

    m = calloc(1, sizeof(*m));

    if (!m) {

        perror("calloc mbuf");

        exit(1);
    }

    m->m_datastart = calloc(1, len);

    if (!m->m_datastart) {

        perror("calloc data");

        exit(1);
    }

    m->m_data = m->m_datastart;

    m->m_len = len;

    m->m_pkthdr.len = len;

    return m;
}

static void m_freem(struct mbuf *m)
{
    if (!m)
        return;

    free(m->m_datastart);

    free(m);
}

/*************************************************************
 * TUN
 *************************************************************/

static int tun_alloc(char *dev)
{
    struct ifreq ifr;

    int fd;

    fd = open("/dev/net/tun", O_RDWR);

    if (fd < 0) {

        perror("open(/dev/net/tun)");

        exit(1);
    }

    memset(&ifr, 0, sizeof(ifr));

    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;

    strncpy(ifr.ifr_name,
            dev,
            IFNAMSIZ);

    if (ioctl(fd, TUNSETIFF, &ifr) < 0) {

        perror("TUNSETIFF");

        exit(1);
    }

    strcpy(dev, ifr.ifr_name);

    return fd;
}

/*************************************************************
 * deterministic payload
 *************************************************************/

static void generate_test_packet(struct test_packet *tp,
                                 uint32_t seq)
{
    tp->magic = htonl(TEST_MAGIC);

    tp->seq = htonl(seq);

    for (int i = 0; i < sizeof(tp->payload); i++) {

        tp->payload[i] = (seq ^ i) & 0xff;
    }

    tp->checksum =
        htonl(payload_checksum(tp->payload,
                               sizeof(tp->payload)));
}

/*************************************************************
 * build IPv4/UDP packet
 *************************************************************/

static void build_ipv4_udp_packet(struct full_packet *pkt,
                                  uint32_t seq)
{
    memset(pkt, 0, sizeof(*pkt));

    generate_test_packet(&pkt->tp, seq);

    /*************************************************
     * IPv4 header
     *************************************************/

    pkt->ip.version = 4;

    pkt->ip.ihl = 5;

    pkt->ip.tos = 0;

    pkt->ip.tot_len =
        htons(sizeof(struct full_packet));

    pkt->ip.id = htons(seq & 0xffff);

    pkt->ip.frag_off = 0;

    pkt->ip.ttl = 64;

    pkt->ip.protocol = IPPROTO_UDP;

    /*
     * IMPORTANT:
     * different subnets
     */

	pkt->ip.saddr = inet_addr("172.16.1.1");
	pkt->ip.daddr = inet_addr("172.16.2.1");

    pkt->ip.check = 0;

    pkt->ip.check =
        ip_checksum(&pkt->ip,
                    sizeof(struct iphdr));

    /*************************************************
     * UDP header
     *************************************************/

    pkt->udp.source = htons(TEST_PORT);

    pkt->udp.dest = htons(TEST_PORT);

    pkt->udp.len =
        htons(sizeof(struct udphdr) +
              sizeof(struct test_packet));

    /*
     * Linux accepts zero UDP checksum for IPv4
     */

    pkt->udp.check = 0;
}

/*************************************************************
 * proto_input()
 *
 * inject packet INTO Linux stack
 *************************************************************/

static void proto_input(struct ifnet *ifp,
                        struct mbuf *m)
{
    ssize_t rc;

    printf("\n=== proto_input() ===\n");

    rc = write(ifp->tun_fd,
               m->m_data,
               m->m_len);

    if (rc < 0) {

        perror("write(tun)");

    } else {

        printf("Injected %ld bytes into kernel stack\n",
               rc);
    }

    m_freem(m);
}

/*************************************************************
 * proto_output()
 *
 * packet FROM Linux stack
 *************************************************************/

static void proto_output(struct ifnet *ifp,
                         struct mbuf *m)
{
    struct iphdr *ip;

    struct udphdr *udp;

    struct test_packet *tp;

    uint32_t calc;

    /*************************************************
     * minimum size check
     *************************************************/

    if (m->m_len <
        (int)(sizeof(struct iphdr) +
              sizeof(struct udphdr))) {

        printf("DROP tiny packet len=%d\n",
               m->m_len);

        m_freem(m);

        return;
    }

    ip = (struct iphdr *)m->m_data;

    printf("\nRX packet len=%d ",
           m->m_len);

    printf("ipver=%d ",
           ip->version);

    printf("proto=%d\n",
           ip->protocol);

    /*************************************************
     * ignore non IPv4
     *************************************************/

    if (ip->version != 4) {

        printf("DROP non-ipv4\n");

        m_freem(m);

        return;
    }

    /*************************************************
     * ignore non UDP
     *************************************************/

    if (ip->protocol != IPPROTO_UDP) {

        printf("DROP non-UDP\n");

        m_freem(m);

        return;
    }

    /*************************************************
     * full payload check
     *************************************************/

    if (m->m_len <
        (int)(sizeof(struct full_packet))) {

        printf("DROP short UDP packet len=%d\n",
               m->m_len);

        m_freem(m);

        return;
    }

    udp = (struct udphdr *)
        (m->m_data + sizeof(struct iphdr));

    printf("UDP srcport=%u dstport=%u\n",
           ntohs(udp->source),
           ntohs(udp->dest));

    /*************************************************
     * ignore unrelated UDP
     *************************************************/

    if (ntohs(udp->dest) != TEST_PORT) {

        printf("DROP unrelated UDP port\n");

        m_freem(m);

        return;
    }

    tp = (struct test_packet *)
        (m->m_data +
         sizeof(struct iphdr) +
         sizeof(struct udphdr));

    /*************************************************
     * validate magic
     *************************************************/

    if (ntohl(tp->magic) != TEST_MAGIC) {

        printf("DROP invalid magic 0x%x\n",
               ntohl(tp->magic));

        m_freem(m);

        return;
    }

    /*************************************************
     * validate payload checksum
     *************************************************/

    calc =
        payload_checksum(tp->payload,
                         sizeof(tp->payload));

    printf("\n=== proto_output() ===\n");

    printf("if=%s\n",
           ifp->if_name);

    printf("seq=%u\n",
           ntohl(tp->seq));

    printf("payload checksum rx=%u calc=%u\n",
           ntohl(tp->checksum),
           calc);

    if (ntohl(tp->checksum) != calc) {

        printf("CHECKSUM FAILURE\n");

    } else {

        printf("CHECKSUM OK\n");
    }

    printf("IPv4 src=%s ",
           inet_ntoa(*(struct in_addr *)&ip->saddr));

    printf("dst=%s\n",
           inet_ntoa(*(struct in_addr *)&ip->daddr));

    m_freem(m);
}

/*************************************************************
 * sender
 *************************************************************/

static void sender_loop(struct proto_instance *pi)
{
    uint32_t seq = 1;

    while (1) {

        struct mbuf *m;

        struct full_packet *pkt;

        m = mbuf_alloc(sizeof(struct full_packet));

        pkt = (struct full_packet *)m->m_data;

        build_ipv4_udp_packet(pkt,
                              seq);

        proto_input(&pi->ifp, m);

        seq++;

        sleep(1);
    }
}

/*************************************************************
 * receiver
 *************************************************************/
static void receiver_loop(struct proto_instance *pi)
{
    while (1) {

        uint8_t buffer[MAX_PKT_SIZE];

        ssize_t n;

        struct mbuf *m;

        struct iphdr *ip;

        /*
         * read packet FROM kernel stack
         */

        n = read(pi->ifp.tun_fd,
                 buffer,
                 sizeof(buffer));

        if (n < 0) {

            perror("read(tun)");

            continue;
        }

        /*
         * minimum IPv4 header size
         */

        if (n < (ssize_t)sizeof(struct iphdr)) {

            printf("DROP tiny packet len=%ld\n", n);

            continue;
        }

        ip = (struct iphdr *)buffer;

        /*
         * ignore non IPv4
         */

        if (ip->version != 4) {

            printf("DROP non-ipv4\n");

            continue;
        }

        /*
         * IMPORTANT FILTER
         *
         * Ignore Linux background traffic.
         *
         * Only accept packets generated by
         * our sender instance.
         */

        if (ip->saddr != inet_addr("172.16.1.1")) {

            printf("IGNORE background packet src=%s\n",
                   inet_ntoa(*(struct in_addr *)&ip->saddr));

            continue;
        }

        /*
         * optional destination validation
         */

        if (ip->daddr != inet_addr("172.16.2.99")) {

            printf("IGNORE unexpected dst=%s\n",
                   inet_ntoa(*(struct in_addr *)&ip->daddr));

            continue;
        }

        /*
         * allocate mbuf
         */

        m = mbuf_alloc(n);

        memcpy(m->m_data,
               buffer,
               n);

        /*
         * emulate legacy:
         *
         * stack
         *   ->
         * proto_output()
         */

        proto_output(&pi->ifp, m);
    }
}

/*
static void receiver_loop_old(struct proto_instance *pi)
{
    while (1) {

        uint8_t buffer[MAX_PKT_SIZE];

        ssize_t n;

        struct mbuf *m;

        n = read(pi->ifp.tun_fd,
                 buffer,
                 sizeof(buffer));

        if (n < 0) {

            perror("read(tun)");

            continue;
        }

        m = mbuf_alloc(n);

        memcpy(m->m_data,
               buffer,
               n);

        proto_output(&pi->ifp, m);
    }
}
* */

/*************************************************************
 * main
 *************************************************************/

int main(int argc, char **argv)
{
    struct proto_instance pi;

    char tun_name[IFNAMSIZ];

    if (argc < 3) {

        printf("usage:\n");

        printf("./proto_harness_v16 A tun0\n");

        printf("./proto_harness_v16 B tun1\n");

        return 1;
    }

    memset(&pi, 0, sizeof(pi));

    snprintf(pi.name,
             sizeof(pi.name),
             "proto-%s",
             argv[1]);

    strcpy(pi.ifp.if_name,
           argv[2]);

    pi.ifp.if_flags =
        IFF_UP | IFF_RUNNING;

    strcpy(tun_name,
           argv[2]);

    pi.ifp.tun_fd =
        tun_alloc(tun_name);

    printf("Created %s\n",
           tun_name);

    if (!strcmp(argv[1], "A")) {

        sender_loop(&pi);

    } else {

        receiver_loop(&pi);
    }

    return 0;
}

