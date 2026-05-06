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
*  
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
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/poll.h>

#define MAX_PKT_SIZE 2048
#define QUEUE_DEPTH  64

#define HALO_NUM_CLP_PRI 2

//#define IFF_UP      0x1
//#define IFF_RUNNING 0x2

typedef uint8_t  u_int_1;
typedef uint16_t u_int_2;
typedef uint32_t u_int_4;

typedef uint8_t L2_addr_t;

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

struct flow_queue {
    u_int_4 last_update;

    int space;

    u_int_2 a_space;

    u_int_1 utilisation;

    struct mbuf *ifq_head;
    struct mbuf *ifq_tail;

    u_int_4 ifq_len;
    u_int_4 ifq_maxlen;
    u_int_4 ifq_drops;
};

typedef struct {
    int pri;
    int discard;
} pd_qos_t;

struct halo_flow;

struct halo_subflow {
    struct halo_subflow *next;
    struct halo_subflow *prev;

    struct flow_queue queue;

    struct halo_flow *parent_flow;

    L2_addr_t l2_addr;
    L2_addr_t src_l2_addr;
};

struct halo_flow {
    struct halo_flow *next;

    struct halo_subflow *subflow[HALO_NUM_CLP_PRI];

    u_int_2 subflow_len[HALO_NUM_CLP_PRI];

    u_int_2 port;

    u_int_2 flow_len;

    pd_qos_t qos;

    u_int_1 utilisation;
};

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

    struct halo_flow default_flow;

    char name[32];
};

struct test_packet {
    uint32_t magic;
    uint32_t seq;
    uint32_t checksum;

    uint8_t payload[256];
};

static uint32_t checksum32(uint8_t *buf, size_t len)
{
    uint32_t sum = 0;

    for (size_t i = 0; i < len; i++)
        sum += buf[i];

    return sum;
}

static struct mbuf *mbuf_alloc(size_t len)
{
    struct mbuf *m = calloc(1, sizeof(*m));

    m->m_datastart = calloc(1, len);

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

    if (*dev)
        strncpy(ifr.ifr_name, dev, IFNAMSIZ);

    if (ioctl(fd, TUNSETIFF, &ifr) < 0) {
        perror("TUNSETIFF");
        exit(1);
    }

    strcpy(dev, ifr.ifr_name);

    return fd;
}

static void queue_enqueue(struct flow_queue *q, struct mbuf *m)
{
    m->m_nextpkt = NULL;

    if (!q->ifq_head)
        q->ifq_head = m;
    else
        q->ifq_tail->m_nextpkt = m;

    q->ifq_tail = m;

    q->ifq_len++;
}

static struct mbuf *queue_dequeue(struct flow_queue *q)
{
    struct mbuf *m = q->ifq_head;

    if (!m)
        return NULL;

    q->ifq_head = m->m_nextpkt;

    if (!q->ifq_head)
        q->ifq_tail = NULL;

    q->ifq_len--;

    return m;
}

static void proto_output(struct ifnet *ifp, struct mbuf *m)
{
    struct test_packet *tp =
        (struct test_packet *)m->m_data;

    uint32_t calc =
        checksum32(tp->payload, sizeof(tp->payload));

    printf("\n=== proto_output() ===\n");

    printf("if=%s len=%d seq=%u\n",
           ifp->if_name,
           m->m_len,
           tp->seq);

    printf("checksum rx=%u calc=%u\n",
           tp->checksum,
           calc);

    if (tp->checksum != calc) {
        printf("CHECKSUM FAILURE\n");
    } else {
        printf("CHECKSUM OK\n");
    }

    m_freem(m);
}

static void proto_input(struct ifnet *ifp, struct mbuf *m)
{
    ssize_t rc;

    printf("\n=== proto_input() ===\n");

    rc = write(ifp->tun_fd,
               m->m_data,
               m->m_len);

    if (rc < 0) {
        perror("write tun");
    } else {
        printf("Injected %ld bytes into kernel stack\n", rc);
    }

    m_freem(m);
}

static void generate_packet(struct mbuf *m, uint32_t seq)
{
    struct test_packet *tp =
        (struct test_packet *)m->m_data;

    tp->magic = 0xdeadbeef;

    tp->seq = seq;

    for (int i = 0; i < sizeof(tp->payload); i++)
        tp->payload[i] = seq ^ i;

    tp->checksum =
        checksum32(tp->payload,
                   sizeof(tp->payload));
}

static void sender_loop(struct proto_instance *pi)
{
    uint32_t seq = 1;

    while (1) {

        struct mbuf *m =
            mbuf_alloc(sizeof(struct test_packet));

        generate_packet(m, seq);

        proto_input(&pi->ifp, m);

        seq++;

        sleep(1);
    }
}

static void receiver_loop(struct proto_instance *pi)
{
    while (1) {

        uint8_t buffer[MAX_PKT_SIZE];

        ssize_t n =
            read(pi->ifp.tun_fd,
                 buffer,
                 sizeof(buffer));

        if (n < 0) {
            perror("read tun");
            continue;
        }

        struct mbuf *m = mbuf_alloc(n);

        memcpy(m->m_data, buffer, n);

        proto_output(&pi->ifp, m);
    }
}

int main(int argc, char **argv)
{
    if (argc < 3) {
        printf("usage:\n");
        printf("./proto_harness A tun0\n");
        printf("./proto_harness B tun1\n");
        return 1;
    }

    struct proto_instance pi;

    memset(&pi, 0, sizeof(pi));

    snprintf(pi.name,
             sizeof(pi.name),
             "proto-%s",
             argv[1]);

    strcpy(pi.ifp.if_name, argv[2]);

    pi.ifp.if_flags =
        IFF_UP | IFF_RUNNING;

    char tun_name[IFNAMSIZ];

    strcpy(tun_name, argv[2]);

    pi.ifp.tun_fd = tun_alloc(tun_name);

    printf("Created %s\n", tun_name);

    if (!strcmp(argv[1], "A"))
        sender_loop(&pi);
    else
        receiver_loop(&pi);

    return 0;
}